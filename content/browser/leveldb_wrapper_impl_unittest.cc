// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/leveldb_wrapper_impl.h"

#include "base/atomic_ref_count.h"
#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread.h"
#include "components/leveldb/public/cpp/util.h"
#include "components/leveldb/public/interfaces/leveldb.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/fake_leveldb_database.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace content {

namespace {

std::string VecToStr(const std::vector<uint8_t>& input) {
  return leveldb::Uint8VectorToStdString(input);
}

std::vector<uint8_t> StrToVec(const std::string& input) {
  return leveldb::StdStringToUint8Vector(input);
}

const char* kTestPrefix = "abc";
const char* kTestKey1 = "def";
const char* kTestValue1 = "defdata";
const char* kTestKey2 = "123";
const char* kTestValue2 = "123data";
const char* kTestCopyPrefix1 = "www";
const char* kTestCopyPrefix2 = "xxx";
const char* kTestCopyPrefix3 = "yyy";
const char* kTestSource = "source";
const size_t kTestSizeLimit = 512;

class InternalIncrementalBarrier {
 public:
  InternalIncrementalBarrier(base::OnceClosure done_closure)
      : num_callbacks_left_(1), done_closure_(std::move(done_closure)) {}

  void Dec() {
    // This is the same as in BarrierClosure.
    DCHECK(!num_callbacks_left_.IsZero());
    if (!num_callbacks_left_.Decrement()) {
      base::OnceClosure done = std::move(done_closure_);
      delete this;
      std::move(done).Run();
    }
  }

  base::OnceClosure Inc() {
    num_callbacks_left_.Increment();
    return base::BindOnce(&InternalIncrementalBarrier::Dec,
                          base::Unretained(this));
  }

 private:
  base::AtomicRefCount num_callbacks_left_;
  base::OnceClosure done_closure_;

  DISALLOW_COPY_AND_ASSIGN(InternalIncrementalBarrier);
};

// We keep a separate InternalIncrementalBarrier class because the done_closure
// needs to run after the destructor of the IncrementalBarrier.
class IncrementalBarrier {
 public:
  explicit IncrementalBarrier(base::OnceClosure done_closure)
      : internal_barrier_(
            new InternalIncrementalBarrier(std::move(done_closure))) {}

  ~IncrementalBarrier() { internal_barrier_->Dec(); }

  base::OnceClosure Get() { return internal_barrier_->Inc(); }

 private:
  InternalIncrementalBarrier* internal_barrier_;  // self-deleting

  DISALLOW_COPY_AND_ASSIGN(IncrementalBarrier);
};

class GetAllCallback : public mojom::LevelDBWrapperGetAllCallback {
 public:
  static mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo CreateAndBind(
      bool* result,
      base::OnceClosure closure) {
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtr ptr;
    auto request = mojo::MakeRequestAssociatedWithDedicatedPipe(&ptr);
    mojo::MakeStrongAssociatedBinding(
        base::WrapUnique(new GetAllCallback(result, std::move(closure))),
        std::move(request));
    return ptr.PassInterface();
  }

 private:
  GetAllCallback(bool* result, base::OnceClosure closure)
      : result_(result), closure_(std::move(closure)) {}
  void Complete(bool success) override {
    *result_ = success;
    std::move(closure_).Run();
  }

  bool* result_;
  base::OnceClosure closure_;
};

class MockDelegate : public LevelDBWrapperImpl::Delegate {
 public:
  void OnNoBindings() override {}
  std::vector<leveldb::mojom::BatchedOperationPtr> PrepareToCommit() override {
    return std::vector<leveldb::mojom::BatchedOperationPtr>();
  }
  void DidCommit(leveldb::mojom::DatabaseError error) override {
    if (committed_)
      std::move(committed_).Run();
  }
  void OnMapLoaded(leveldb::mojom::DatabaseError error) override {
    map_load_count_++;
  }
  std::vector<LevelDBWrapperImpl::Change> FixUpData(
      const LevelDBWrapperImpl::ValueMap& data) override {
    return mock_changes_;
  }

  int map_load_count() const { return map_load_count_; }
  void set_mock_changes(std::vector<LevelDBWrapperImpl::Change> changes) {
    mock_changes_ = std::move(changes);
  }

  void SetDidCommitCallback(base::OnceClosure committed) {
    committed_ = std::move(committed);
  }

 private:
  int map_load_count_ = 0;
  std::vector<LevelDBWrapperImpl::Change> mock_changes_;
  base::OnceClosure committed_;
};

void GetCallback(base::OnceClosure closure,
                 bool* success_out,
                 std::vector<uint8_t>* value_out,
                 bool success,
                 const std::vector<uint8_t>& value) {
  *success_out = success;
  *value_out = value;
  std::move(closure).Run();
}

void GetAllDataCallback(leveldb::mojom::DatabaseError* status_out,
                        std::vector<mojom::KeyValuePtr>* data_out,
                        leveldb::mojom::DatabaseError status,
                        std::vector<mojom::KeyValuePtr> data) {
  *status_out = status;
  *data_out = std::move(data);
}

void SuccessCallback(base::OnceClosure closure,
                     bool* success_out,
                     bool success) {
  *success_out = success;
  std::move(closure).Run();
}

void NoOpSuccessCallback(bool success) {}

void WrapperCallback(base::OnceClosure done,
                     std::unique_ptr<LevelDBWrapperImpl>* wrapper_out,
                     std::unique_ptr<LevelDBWrapperImpl> wrapper) {
  *wrapper_out = std::move(wrapper);
  std::move(done).Run();
}

void MaybeCommitChanges(MockDelegate* delegate,
                        LevelDBWrapperImpl* wrapper,
                        base::OnceClosure done) {
  if (!wrapper->has_pending_load_tasks() && !wrapper->has_changes_to_commit()) {
    std::move(done).Run();
  }
  delegate->SetDidCommitCallback(std::move(done));
  wrapper->ScheduleImmediateCommit();
}

void ScheduleCommitChanges(MockDelegate* delegate,
                           LevelDBWrapperImpl* wrapper) {
  base::RunLoop loop;
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MaybeCommitChanges, delegate, wrapper,
                                loop.QuitClosure()));
  loop.Run();
}

}  // namespace

class LevelDBWrapperImplTest : public testing::Test,
                               public mojom::LevelDBObserver {
 public:
  struct Observation {
    enum { kAdd, kChange, kDelete, kDeleteAll } type;
    std::string key;
    std::string old_value;
    std::string new_value;
    std::string source;
  };

  LevelDBWrapperImplTest() : db_(&mock_data_), observer_binding_(this) {
    auto request = mojo::MakeRequest(&level_db_database_ptr_);
    db_.Bind(std::move(request));

    level_db_wrapper_.reset(
        new LevelDBWrapperImpl(level_db_database_ptr_.get(), kTestPrefix,
                               kTestSizeLimit, base::TimeDelta::FromSeconds(5),
                               10 * 1024 * 1024 /* max_bytes_per_hour */,
                               60 /* max_commits_per_hour */, &delegate_));

    set_mock_data(std::string(kTestPrefix) + kTestKey1, kTestValue1);
    set_mock_data(std::string(kTestPrefix) + kTestKey2, kTestValue2);
    set_mock_data("123", "baddata");

    level_db_wrapper_->Bind(mojo::MakeRequest(&level_db_wrapper_ptr_));
    mojom::LevelDBObserverAssociatedPtrInfo ptr_info;
    observer_binding_.Bind(mojo::MakeRequest(&ptr_info));
    level_db_wrapper_ptr_->AddObserver(std::move(ptr_info));
  }

  ~LevelDBWrapperImplTest() override {
    //    database_binding_.Close();
  }

  void set_mock_data(const std::string& key, const std::string& value) {
    mock_data_[StrToVec(key)] = StrToVec(value);
  }

  void set_mock_data(const std::vector<uint8_t>& key,
                     const std::vector<uint8_t>& value) {
    mock_data_[key] = value;
  }

  bool has_mock_data(const std::string& key) {
    return mock_data_.find(StrToVec(key)) != mock_data_.end();
  }

  std::string get_mock_data(const std::string& key) {
    return has_mock_data(key) ? VecToStr(mock_data_[StrToVec(key)]) : "";
  }

  void clear_mock_data() { mock_data_.clear(); }

  mojom::LevelDBWrapper* wrapper() { return level_db_wrapper_ptr_.get(); }
  LevelDBWrapperImpl* wrapper_impl() { return level_db_wrapper_.get(); }

  leveldb::mojom::LevelDBDatabase* database() {
    return level_db_database_ptr_.get();
  }

  void Get(mojom::LevelDBWrapper* wrapper,
           const std::vector<uint8_t>& key,
           bool* success,
           std::vector<uint8_t>* result,
           base::OnceClosure done) {
    wrapper->Get(
        key, base::BindOnce(&GetCallback, std::move(done), success, result));
  }
  void Put(mojom::LevelDBWrapper* wrapper,
           const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           bool* success,
           base::OnceClosure done,
           std::string source = kTestSource) {
    wrapper->Put(key, value, client_old_value, source,
                 base::BindOnce(&SuccessCallback, std::move(done), success));
  }
  void Delete(mojom::LevelDBWrapper* wrapper,
              const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              bool* success,
              base::OnceClosure done) {
    wrapper->Delete(key, client_old_value, kTestSource,
                    base::BindOnce(&SuccessCallback, std::move(done), success));
  }

  void DeleteAll(mojom::LevelDBWrapper* wrapper,
                 bool* success,
                 base::OnceClosure done) {
    wrapper->DeleteAll(kTestSource, base::BindOnce(&SuccessCallback,
                                                   std::move(done), success));
  }

  void Get(const std::vector<uint8_t>& key,
           bool* success,
           std::vector<uint8_t>* result,
           base::OnceClosure done) {
    Get(wrapper(), key, success, result, std::move(done));
  }

  void Put(const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           bool* success,
           base::OnceClosure done,
           std::string source = kTestSource) {
    Put(wrapper(), key, value, client_old_value, success, std::move(done),
        source);
  }

  void Delete(const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              bool* success,
              base::OnceClosure done) {
    Delete(wrapper(), key, client_old_value, success, std::move(done));
  }

  void DeleteAll(bool* success, base::OnceClosure done) {
    DeleteAll(wrapper(), success, std::move(done));
  }

  bool GetSync(mojom::LevelDBWrapper* wrapper,
               const std::vector<uint8_t>& key,
               std::vector<uint8_t>* result) {
    bool success = false;
    base::RunLoop loop;
    Get(wrapper, key, &success, result, loop.QuitClosure());
    loop.Run();
    return success;
  }

  std::vector<uint8_t> GetVecSync(mojom::LevelDBWrapper* wrapper,
                                  const std::vector<uint8_t>& key) {
    std::vector<uint8_t> result;
    bool success = GetSync(wrapper, key, &result);
    return success ? result : std::vector<uint8_t>();
  }

  std::string GetStrSync(mojom::LevelDBWrapper* wrapper,
                         const std::string& key) {
    return VecToStr(GetVecSync(wrapper, StrToVec(key)));
  }

  bool PutSync(mojom::LevelDBWrapper* wrapper,
               const std::vector<uint8_t>& key,
               const std::vector<uint8_t>& value,
               const base::Optional<std::vector<uint8_t>>& client_old_value,
               std::string source = kTestSource) {
    bool success = false;
    base::RunLoop loop;
    Put(wrapper, key, value, client_old_value, &success, loop.QuitClosure(),
        source);
    loop.Run();
    return success;
  }

  bool DeleteSync(
      mojom::LevelDBWrapper* wrapper,
      const std::vector<uint8_t>& key,
      const base::Optional<std::vector<uint8_t>>& client_old_value) {
    bool success = false;
    base::RunLoop loop;
    Delete(wrapper, key, client_old_value, &success, loop.QuitClosure());
    loop.Run();
    return success;
  }

  bool DeleteAllSync(mojom::LevelDBWrapper* wrapper) {
    bool success = false;
    base::RunLoop loop;
    DeleteAll(wrapper, &success, loop.QuitClosure());
    loop.Run();
    return success;
  }

  bool GetSync(const std::vector<uint8_t>& key, std::vector<uint8_t>* result) {
    return GetSync(wrapper(), key, result);
  }

  bool PutSync(const std::vector<uint8_t>& key,
               const std::vector<uint8_t>& value,
               const base::Optional<std::vector<uint8_t>>& client_old_value,
               std::string source = kTestSource) {
    return PutSync(wrapper(), key, value, client_old_value, source);
  }

  bool DeleteSync(
      const std::vector<uint8_t>& key,
      const base::Optional<std::vector<uint8_t>>& client_old_value) {
    return DeleteSync(wrapper(), key, client_old_value);
  }

  bool DeleteAllSync() { return DeleteAllSync(wrapper()); }

  bool SyncWrapperHasValue(mojom::LevelDBWrapper* wrapper,
                           const std::string& str_key,
                           const std::string& str_expected_result) {
    bool success = false;
    base::RunLoop loop;
    std::vector<uint8_t> result;
    Get(wrapper, StrToVec(str_key), &success, &result, loop.QuitClosure());
    loop.Run();
    return success && VecToStr(result) == str_expected_result;
  }

  void ScheduleForkToPrefix(LevelDBWrapperImpl* source,
                            const std::string& new_prefix,
                            LevelDBWrapperImpl::Delegate* delegate,
                            std::unique_ptr<LevelDBWrapperImpl>* destination,
                            base::OnceClosure done) {
    base::PostTaskAndReplyWithResult(
        base::SequencedTaskRunnerHandle::Get().get(), FROM_HERE,
        base::BindOnce(&LevelDBWrapperImpl::ForkToNewPrefix,
                       base::Unretained(source), new_prefix, delegate),
        base::BindOnce(&WrapperCallback, base::Passed(&done), destination));
  }

  void CommitChanges() { CommitChanges(&delegate_, level_db_wrapper_.get()); }

  void CommitChanges(MockDelegate* delegate, LevelDBWrapperImpl* wrapper) {
    if (!wrapper->has_pending_load_tasks() && !wrapper->has_changes_to_commit())
      return;
    base::RunLoop loop;
    delegate->SetDidCommitCallback(loop.QuitClosure());
    wrapper->ScheduleImmediateCommit();
    loop.Run();
  }

  const std::vector<Observation>& observations() { return observations_; }

  MockDelegate* delegate() { return &delegate_; }

 private:
  // LevelDBObserver:
  void KeyAdded(const std::vector<uint8_t>& key,
                const std::vector<uint8_t>& value,
                const std::string& source) override {
    observations_.push_back(
        {Observation::kAdd, VecToStr(key), "", VecToStr(value), source});
  }
  void KeyChanged(const std::vector<uint8_t>& key,
                  const std::vector<uint8_t>& new_value,
                  const std::vector<uint8_t>& old_value,
                  const std::string& source) override {
    observations_.push_back({Observation::kChange, VecToStr(key),
                             VecToStr(old_value), VecToStr(new_value), source});
  }
  void KeyDeleted(const std::vector<uint8_t>& key,
                  const std::vector<uint8_t>& old_value,
                  const std::string& source) override {
    observations_.push_back(
        {Observation::kDelete, VecToStr(key), VecToStr(old_value), "", source});
  }
  void AllDeleted(const std::string& source) override {
    observations_.push_back({Observation::kDeleteAll, "", "", "", source});
  }
  void ShouldSendOldValueOnMutations(bool value) override {}

  TestBrowserThreadBundle thread_bundle_;
  std::map<std::vector<uint8_t>, std::vector<uint8_t>> mock_data_;
  FakeLevelDBDatabase db_;
  leveldb::mojom::LevelDBDatabasePtr level_db_database_ptr_;
  MockDelegate delegate_;
  std::unique_ptr<LevelDBWrapperImpl> level_db_wrapper_;
  mojom::LevelDBWrapperPtr level_db_wrapper_ptr_;
  mojo::AssociatedBinding<mojom::LevelDBObserver> observer_binding_;
  std::vector<Observation> observations_;
};

TEST_F(LevelDBWrapperImplTest, GetLoadedFromMap) {
  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(StrToVec("123"), &result));
  EXPECT_EQ(StrToVec("123data"), result);

  EXPECT_FALSE(GetSync(StrToVec("x"), &result));
}

TEST_F(LevelDBWrapperImplTest, GetFromPutOverwrite) {
  std::vector<uint8_t> key = StrToVec("123");
  std::vector<uint8_t> value = StrToVec("foo");

  base::RunLoop loop;
  bool put_success = false;
  base::RepeatingClosure barrier = base::BarrierClosure(2, loop.QuitClosure());
  Put(key, value, StrToVec("123data"), &put_success, barrier);

  std::vector<uint8_t> result;
  bool get_success = false;
  Get(key, &get_success, &result, barrier);

  loop.Run();
  EXPECT_TRUE(put_success);
  EXPECT_TRUE(get_success);

  EXPECT_EQ(value, result);
}

TEST_F(LevelDBWrapperImplTest, GetFromPutNewKey) {
  std::vector<uint8_t> key = StrToVec("newkey");
  std::vector<uint8_t> value = StrToVec("foo");

  EXPECT_TRUE(PutSync(key, value, base::nullopt));

  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(key, &result));
  EXPECT_EQ(value, result);
}

TEST_F(LevelDBWrapperImplTest, GetAll) {
  leveldb::mojom::DatabaseError status;
  std::vector<mojom::KeyValuePtr> data;
  bool result = false;

  base::RunLoop loop;
  // Testing the 'Sync' mojo version is a big headache involving 3 threads, so
  // just test the async version.
  wrapper_impl()->GetAll(
      GetAllCallback::CreateAndBind(&result, loop.QuitClosure()),
      base::BindOnce(&GetAllDataCallback, &status, &data));
  loop.Run();
  EXPECT_EQ(leveldb::mojom::DatabaseError::OK, status);
  EXPECT_EQ(2u, data.size());
  EXPECT_TRUE(result);
}

TEST_F(LevelDBWrapperImplTest, CommitPutToDB) {
  std::string key1 = "123";
  std::string value1 = "foo";
  std::string key2 = "abc";
  std::string value2 = "data abc";

  base::RunLoop loop;
  bool put_success1 = false;
  bool put_success2 = false;
  bool put_success3 = false;
  base::RepeatingClosure barrier = base::BarrierClosure(3, loop.QuitClosure());

  Put(StrToVec(key1), StrToVec(value1), StrToVec("123data"), &put_success1,
      barrier);
  Put(StrToVec(key2), StrToVec("old value"), base::nullopt, &put_success2,
      barrier);
  Put(StrToVec(key2), StrToVec(value2), StrToVec("old value"), &put_success3,
      barrier);

  loop.Run();
  EXPECT_TRUE(put_success1 && put_success2 && put_success3);

  EXPECT_FALSE(has_mock_data(kTestPrefix + key2));

  CommitChanges();

  EXPECT_TRUE(has_mock_data(kTestPrefix + key1));
  EXPECT_EQ(value1, get_mock_data(kTestPrefix + key1));
  EXPECT_TRUE(has_mock_data(kTestPrefix + key2));
  EXPECT_EQ(value2, get_mock_data(kTestPrefix + key2));
}

TEST_F(LevelDBWrapperImplTest, PutObservations) {
  std::string key = "new_key";
  std::string value1 = "foo";
  std::string value2 = "data abc";
  std::string source1 = "source1";
  std::string source2 = "source2";

  EXPECT_TRUE(PutSync(StrToVec(key), StrToVec(value1), base::nullopt, source1));
  ASSERT_EQ(1u, observations().size());
  EXPECT_EQ(Observation::kAdd, observations()[0].type);
  EXPECT_EQ(key, observations()[0].key);
  EXPECT_EQ(value1, observations()[0].new_value);
  EXPECT_EQ(source1, observations()[0].source);

  EXPECT_TRUE(
      PutSync(StrToVec(key), StrToVec(value2), StrToVec(value1), source2));
  ASSERT_EQ(2u, observations().size());
  EXPECT_EQ(Observation::kChange, observations()[1].type);
  EXPECT_EQ(key, observations()[1].key);
  EXPECT_EQ(value1, observations()[1].old_value);
  EXPECT_EQ(value2, observations()[1].new_value);
  EXPECT_EQ(source2, observations()[1].source);

  // Same put should not cause another observation.
  EXPECT_TRUE(PutSync(StrToVec(key), StrToVec(value2), base::nullopt, source2));
  ASSERT_EQ(2u, observations().size());
}

TEST_F(LevelDBWrapperImplTest, DeleteNonExistingKey) {
  EXPECT_TRUE(DeleteSync(StrToVec("doesn't exist"), std::vector<uint8_t>()));
  EXPECT_EQ(0u, observations().size());
}

TEST_F(LevelDBWrapperImplTest, DeleteExistingKey) {
  std::string key = "newkey";
  std::string value = "foo";
  set_mock_data(kTestPrefix + key, value);

  EXPECT_TRUE(DeleteSync(StrToVec(key), StrToVec(value)));
  ASSERT_EQ(1u, observations().size());
  EXPECT_EQ(Observation::kDelete, observations()[0].type);
  EXPECT_EQ(key, observations()[0].key);
  EXPECT_EQ(value, observations()[0].old_value);
  EXPECT_EQ(kTestSource, observations()[0].source);

  EXPECT_TRUE(has_mock_data(kTestPrefix + key));

  CommitChanges();
  EXPECT_FALSE(has_mock_data(kTestPrefix + key));
}

TEST_F(LevelDBWrapperImplTest, DeleteAllWithoutLoadedMap) {
  std::string key = "newkey";
  std::string value = "foo";
  std::string dummy_key = "foobar";
  set_mock_data(dummy_key, value);
  set_mock_data(kTestPrefix + key, value);

  EXPECT_TRUE(DeleteAllSync());
  ASSERT_EQ(1u, observations().size());
  EXPECT_EQ(Observation::kDeleteAll, observations()[0].type);
  EXPECT_EQ(kTestSource, observations()[0].source);

  EXPECT_TRUE(has_mock_data(kTestPrefix + key));
  EXPECT_TRUE(has_mock_data(dummy_key));

  CommitChanges();
  EXPECT_FALSE(has_mock_data(kTestPrefix + key));
  EXPECT_TRUE(has_mock_data(dummy_key));

  // Deleting all again should still work, but not cause an observation.
  EXPECT_TRUE(DeleteAllSync());
  ASSERT_EQ(1u, observations().size());

  // And now we've deleted all, writing something the quota size should work.
  EXPECT_TRUE(PutSync(std::vector<uint8_t>(kTestSizeLimit, 'b'),
                      std::vector<uint8_t>(), base::nullopt));
}

TEST_F(LevelDBWrapperImplTest, DeleteAllWithLoadedMap) {
  std::string key = "newkey";
  std::string value = "foo";
  std::string dummy_key = "foobar";
  set_mock_data(dummy_key, value);

  EXPECT_TRUE(PutSync(StrToVec(key), StrToVec(value), base::nullopt));

  EXPECT_TRUE(DeleteAllSync());
  ASSERT_EQ(2u, observations().size());
  EXPECT_EQ(Observation::kDeleteAll, observations()[1].type);
  EXPECT_EQ(kTestSource, observations()[1].source);

  EXPECT_TRUE(has_mock_data(dummy_key));

  CommitChanges();
  EXPECT_FALSE(has_mock_data(kTestPrefix + key));
  EXPECT_TRUE(has_mock_data(dummy_key));
}

TEST_F(LevelDBWrapperImplTest, DeleteAllWithPendingMapLoad) {
  std::string key = "newkey";
  std::string value = "foo";
  std::string dummy_key = "foobar";
  set_mock_data(dummy_key, value);

  wrapper()->Put(StrToVec(key), StrToVec(value), base::nullopt, kTestSource,
                 base::BindOnce(&NoOpSuccessCallback));

  EXPECT_TRUE(DeleteAllSync());
  ASSERT_EQ(2u, observations().size());
  EXPECT_EQ(Observation::kDeleteAll, observations()[1].type);
  EXPECT_EQ(kTestSource, observations()[1].source);

  EXPECT_TRUE(has_mock_data(dummy_key));

  CommitChanges();
  EXPECT_FALSE(has_mock_data(kTestPrefix + key));
  EXPECT_TRUE(has_mock_data(dummy_key));
}

TEST_F(LevelDBWrapperImplTest, DeleteAllWithoutLoadedEmptyMap) {
  clear_mock_data();

  EXPECT_TRUE(DeleteAllSync());
  ASSERT_EQ(0u, observations().size());
}

TEST_F(LevelDBWrapperImplTest, PutOverQuotaLargeValue) {
  std::vector<uint8_t> key = StrToVec("newkey");
  std::vector<uint8_t> value(kTestSizeLimit, 4);

  EXPECT_FALSE(PutSync(key, value, base::nullopt));

  value.resize(kTestSizeLimit / 2);
  EXPECT_TRUE(PutSync(key, value, base::nullopt));
}

TEST_F(LevelDBWrapperImplTest, PutOverQuotaLargeKey) {
  std::vector<uint8_t> key(kTestSizeLimit, 'a');
  std::vector<uint8_t> value = StrToVec("newvalue");

  EXPECT_FALSE(PutSync(key, value, base::nullopt));

  key.resize(kTestSizeLimit / 2);
  EXPECT_TRUE(PutSync(key, value, base::nullopt));
}

TEST_F(LevelDBWrapperImplTest, PutWhenAlreadyOverQuota) {
  std::string key = "largedata";
  std::vector<uint8_t> value(kTestSizeLimit, 4);
  std::vector<uint8_t> old_value = value;

  set_mock_data(kTestPrefix + key, VecToStr(value));

  // Put with same data should succeed.
  EXPECT_TRUE(PutSync(StrToVec(key), value, base::nullopt));

  // Put with same data size should succeed.
  value[1] = 13;
  EXPECT_TRUE(PutSync(StrToVec(key), value, old_value));

  // Adding a new key when already over quota should not succeed.
  EXPECT_FALSE(PutSync(StrToVec("newkey"), {1, 2, 3}, base::nullopt));

  // Reducing size should also succeed.
  old_value = value;
  value.resize(kTestSizeLimit / 2);
  EXPECT_TRUE(PutSync(StrToVec(key), value, old_value));

  // Increasing size again should succeed, as still under the limit.
  old_value = value;
  value.resize(value.size() + 1);
  EXPECT_TRUE(PutSync(StrToVec(key), value, old_value));

  // But increasing back to original size should fail.
  old_value = value;
  value.resize(kTestSizeLimit);
  EXPECT_FALSE(PutSync(StrToVec(key), value, old_value));
}

TEST_F(LevelDBWrapperImplTest, PutWhenAlreadyOverQuotaBecauseOfLargeKey) {
  std::vector<uint8_t> key(kTestSizeLimit, 'x');
  std::vector<uint8_t> value = StrToVec("value");
  std::vector<uint8_t> old_value = value;

  set_mock_data(kTestPrefix + VecToStr(key), VecToStr(value));

  // Put with same data size should succeed.
  value[0] = 'X';
  EXPECT_TRUE(PutSync(key, value, old_value));

  // Reducing size should also succeed.
  old_value = value;
  value.clear();
  EXPECT_TRUE(PutSync(key, value, old_value));

  // Increasing size should fail.
  old_value = value;
  value.resize(1, 'a');
  EXPECT_FALSE(PutSync(key, value, old_value));
}

TEST_F(LevelDBWrapperImplTest, GetAfterPurgeMemory) {
  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(StrToVec("123"), &result));
  EXPECT_EQ(StrToVec("123data"), result);
  EXPECT_EQ(delegate()->map_load_count(), 1);

  // Reading again doesn't load map again.
  EXPECT_TRUE(GetSync(StrToVec("123"), &result));
  EXPECT_EQ(delegate()->map_load_count(), 1);

  wrapper_impl()->PurgeMemory();

  // Now reading should still work, and load map again.
  result.clear();
  EXPECT_TRUE(GetSync(StrToVec("123"), &result));
  EXPECT_EQ(StrToVec("123data"), result);
  EXPECT_EQ(delegate()->map_load_count(), 2);
}

TEST_F(LevelDBWrapperImplTest, PurgeMemoryWithPendingChanges) {
  std::vector<uint8_t> key = StrToVec("123");
  std::vector<uint8_t> value = StrToVec("foo");
  EXPECT_TRUE(PutSync(key, value, StrToVec("123data")));
  EXPECT_EQ(delegate()->map_load_count(), 1);

  // Purge memory, and read. Should not actually have purged, so should not have
  // triggered a load.
  wrapper_impl()->PurgeMemory();

  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(key, &result));
  EXPECT_EQ(value, result);
  EXPECT_EQ(delegate()->map_load_count(), 1);
}

TEST_F(LevelDBWrapperImplTest, FixUpData) {
  std::vector<LevelDBWrapperImpl::Change> changes;
  changes.push_back(std::make_pair(StrToVec("def"), StrToVec("foo")));
  changes.push_back(std::make_pair(StrToVec("123"), base::nullopt));
  changes.push_back(std::make_pair(StrToVec("abc"), StrToVec("bla")));
  delegate()->set_mock_changes(std::move(changes));

  std::vector<uint8_t> result;
  EXPECT_FALSE(GetSync(StrToVec("123"), &result));
  EXPECT_TRUE(GetSync(StrToVec("def"), &result));
  EXPECT_EQ(StrToVec("foo"), result);
  EXPECT_TRUE(GetSync(StrToVec("abc"), &result));
  EXPECT_EQ(StrToVec("bla"), result);

  EXPECT_FALSE(has_mock_data(kTestPrefix + std::string("123")));
  EXPECT_EQ("foo", get_mock_data(kTestPrefix + std::string("def")));
  EXPECT_EQ("bla", get_mock_data(kTestPrefix + std::string("abc")));
}

TEST_F(LevelDBWrapperImplTest, PrefixForking) {
  std::string value3 = "value3";
  std::string value4 = "value4";
  std::string value5 = "value5";

  // In order to test the interaction between forking and mojo calls where
  // forking can happen in between a request and reply to the wrapper mojo
  // service
  // in between the

  // Operations in the same run cycle:
  // Fork 1 created from original
  // Put on fork 1
  // Fork 2 create from fork 1
  // Put on fork 1
  // Put on original
  // Fork 3 created from original
  std::unique_ptr<LevelDBWrapperImpl> fork1;
  MockDelegate fork1_delegate;
  std::unique_ptr<LevelDBWrapperImpl> fork2;
  MockDelegate fork2_delegate;
  std::unique_ptr<LevelDBWrapperImpl> fork3;
  MockDelegate fork3_delegate;
  {
    bool put_success1 = false;
    bool put_success2 = false;
    bool put_success3 = false;
    base::RunLoop loop;
    base::RepeatingClosure barrier =
        base::BarrierClosure(4, loop.QuitClosure());

    // Create fork 1.
    fork1 = wrapper_impl()->ForkToNewPrefix(kTestCopyPrefix1, &fork1_delegate);

    // Do a put on fork 1 and create fork 2.
    // Note - these are 'skipping' the mojo layer, which is why the fork isn't
    // scheduled.
    Put(fork1.get(), StrToVec(kTestKey2), StrToVec(value4),
        StrToVec(kTestValue2), &put_success1, barrier);
    fork2 = fork1->ForkToNewPrefix(kTestCopyPrefix2, &fork2_delegate);
    Put(fork1.get(), StrToVec(kTestKey2), StrToVec(value5), StrToVec(value4),
        &put_success2, barrier);

    // Do a put on original and create fork 3.
    Put(StrToVec(kTestKey1), StrToVec(value3), StrToVec(kTestValue1),
        &put_success3, barrier);
    ScheduleForkToPrefix(wrapper_impl(), kTestCopyPrefix3, &fork3_delegate,
                         &fork3, barrier);

    loop.Run();
    EXPECT_TRUE(put_success1);
    EXPECT_TRUE(put_success2);
    EXPECT_TRUE(fork2.get());
    EXPECT_TRUE(fork3.get());
  }

  // Test values are accurate before commit
  EXPECT_EQ(value3, GetStrSync(wrapper(), kTestKey1));
  EXPECT_EQ(kTestValue1, GetStrSync(fork1.get(), kTestKey1));
  EXPECT_EQ(kTestValue1, GetStrSync(fork2.get(), kTestKey1));
  EXPECT_EQ(value3, GetStrSync(fork3.get(), kTestKey1));
  EXPECT_EQ(kTestValue2, GetStrSync(wrapper(), kTestKey2));
  EXPECT_EQ(value5, GetStrSync(fork1.get(), kTestKey2));
  EXPECT_EQ(value4, GetStrSync(fork2.get(), kTestKey2));
  EXPECT_EQ(kTestValue2, GetStrSync(fork3.get(), kTestKey2));

  ScheduleCommitChanges(delegate(), wrapper_impl());
  ScheduleCommitChanges(&fork1_delegate, fork1.get());

  // kTestKey1 values.
  EXPECT_EQ(value3, get_mock_data(std::string(kTestPrefix) + kTestKey1));
  EXPECT_EQ(kTestValue1,
            get_mock_data(std::string(kTestCopyPrefix1) + kTestKey1));
  EXPECT_EQ(kTestValue1,
            get_mock_data(std::string(kTestCopyPrefix2) + kTestKey1));
  EXPECT_EQ(value3, get_mock_data(std::string(kTestCopyPrefix3) + kTestKey1));

  // kTestKey2 values.
  EXPECT_EQ(kTestValue2, get_mock_data(std::string(kTestPrefix) + kTestKey2));
  EXPECT_EQ(value5, get_mock_data(std::string(kTestCopyPrefix1) + kTestKey2));
  EXPECT_EQ(value4, get_mock_data(std::string(kTestCopyPrefix2) + kTestKey2));
  EXPECT_EQ(kTestValue2,
            get_mock_data(std::string(kTestCopyPrefix3) + kTestKey2));
}

TEST_F(LevelDBWrapperImplTest, PrefixForkAfterLoad) {
  const std::string kValue = "foo";
  const std::vector<uint8_t> kValueVec = StrToVec(kValue);

  // Do a sync put so the map loads.
  EXPECT_TRUE(PutSync(StrToVec(kTestKey1), kValueVec, base::nullopt));

  // Execute the fork.
  MockDelegate fork1_delegate;
  std::unique_ptr<LevelDBWrapperImpl> fork1 =
      wrapper_impl()->ForkToNewPrefix(kTestCopyPrefix1, &fork1_delegate);

  // Check our forked state.
  EXPECT_EQ(kValue, GetStrSync(fork1.get(), kTestKey1));

  ScheduleCommitChanges(delegate(), wrapper_impl());

  EXPECT_EQ(kValue, get_mock_data(std::string(kTestCopyPrefix1) + kTestKey1));
}

std::string GetPrefix(int64_t* i) {
  (*i)++;
  return "prefix-" + base::Int64ToString(*i) + "-";
}

std::vector<uint8_t> VecWithElement(uint8_t element) {
  std::vector<uint8_t> vec = {element};
  return vec;
}

struct FuzzState {
  bool has_val1 = false;
  uint8_t val1;
  bool has_val2 = false;
  uint8_t val2;
};

TEST_F(LevelDBWrapperImplTest, PrefixForkingPsuedoFuzzer) {
  const std::string kKey1 = "key1";
  const std::vector<uint8_t> kKey1Vec = StrToVec(kKey1);
  const std::string kKey2 = "key2";
  const std::vector<uint8_t> kKey2Vec = StrToVec(kKey2);

  std::map<int64_t, FuzzState> states;
  std::map<int64_t, std::unique_ptr<LevelDBWrapperImpl>> wrappers;
  std::map<int64_t, MockDelegate> delegates;
  std::list<bool> successes;
  int64_t curr_prefix = 0;

  base::RunLoop loop;
  {
    IncrementalBarrier barrier(loop.QuitClosure());
    for (int64_t i = 0; i < 1000; i++) {
      FuzzState& state = states[i];
      if (!wrappers[i]) {
        delegates[i] = MockDelegate();
        wrappers[i] = wrapper_impl()->ForkToNewPrefix(GetPrefix(&curr_prefix),
                                                      &delegates[i]);
      }
      int64_t forks = i;
      if (i % 5 == 0 || i % 6 == 0) {
        forks++;
        delegates[forks] = MockDelegate();
        states[forks] = state;
        wrappers[forks] = wrappers[i]->ForkToNewPrefix(GetPrefix(&curr_prefix),
                                                       &delegates[forks]);
      }
      if (i % 13 == 0) {
        state.has_val1 = false;
        successes.push_back(false);
        Delete(wrappers[i].get(), kKey1Vec, std::vector<uint8_t>(),
               &successes.back(), barrier.Get());
      }
      if (i % 4 == 0) {
        state.val2 = i;
        state.has_val2 = true;
        successes.push_back(false);
        Put(wrappers[i].get(), kKey2Vec, {state.val2}, std::vector<uint8_t>(),
            &successes.back(), barrier.Get());
      }
      if (i % 3 == 0) {
        state.val1 = i + 5;
        state.has_val1 = true;
        successes.push_back(false);
        Put(wrappers[i].get(), kKey1Vec, {state.val1}, std::vector<uint8_t>(),
            &successes.back(), barrier.Get());
      }
      if (i % 11 == 0) {
        state.has_val1 = false;
        state.has_val2 = false;
        successes.push_back(false);
        DeleteAll(wrappers[i].get(), &successes.back(), barrier.Get());
      }
      if (i % 2 == 0) {
        forks++;
        delegates[forks] = MockDelegate();
        states[forks] = state;
        wrappers[forks] = wrappers[i]->ForkToNewPrefix(GetPrefix(&curr_prefix),
                                                       &delegates[forks]);
      }
      if (i % 3 == 0) {
        state.val1 = i + 9;
        state.has_val1 = true;
        successes.push_back(false);
        Put(wrappers[i].get(), kKey1Vec, {state.val1}, std::vector<uint8_t>(),
            &successes.back(), barrier.Get());
      }
    }
  }
  loop.Run();

  // Check our states before commit.
  size_t total = wrappers.size();
  for (size_t i = 0; i < total; i++) {
    FuzzState& state = states[i];
    std::vector<uint8_t> result;
    EXPECT_EQ(state.has_val1, GetSync(wrappers[i].get(), kKey1Vec, &result))
        << i;
    if (state.has_val1)
      EXPECT_EQ(state.val1, result[0]);
    EXPECT_EQ(state.has_val2, GetSync(wrappers[i].get(), kKey2Vec, &result))
        << i;
    if (state.has_val2)
      EXPECT_EQ(state.val2, result[0]);
  }

  ASSERT_EQ(wrappers.size(), delegates.size());
  size_t half = total / 2;
  for (size_t i = 0; i < half; i++) {
    CommitChanges(&delegates[i], wrappers[i].get());
  }

  for (size_t i = total - 1; i >= half; i--) {
    CommitChanges(&delegates[i], wrappers[i].get());
  }

  // Check our database
  for (size_t i = 0; i < total; ++i) {
    FuzzState& state = states[i];

    std::vector<uint8_t> prefix = wrappers[i]->prefix();
    auto val1 = VecToStr(VecWithElement(state.val1));
    auto val2 = VecToStr(VecWithElement(state.val2));
    std::string key1 = VecToStr(prefix) + kKey1;
    std::string key2 = VecToStr(prefix) + kKey2;
    EXPECT_EQ(state.has_val1, has_mock_data(key1));
    if (state.has_val1)
      EXPECT_EQ(val1, get_mock_data(key1));
    EXPECT_EQ(state.has_val2, has_mock_data(key2));
    if (state.has_val2)
      EXPECT_EQ(val2, get_mock_data(key2));

    EXPECT_FALSE(wrappers[i]->has_pending_load_tasks()) << i;
  }
}

}  // namespace content
