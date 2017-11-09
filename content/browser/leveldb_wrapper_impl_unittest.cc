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
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {
using CacheMode = LevelDBWrapperImpl::CacheMode;

std::string VecToStr(const std::vector<uint8_t>& input) {
  return leveldb::Uint8VectorToStdString(input);
}

std::vector<uint8_t> StrToVec(const std::string& input) {
  return leveldb::StdStringToUint8Vector(input);
}

const std::string kTestPrefix = "abc";
const std::string kTestKey1 = "def";
const std::string kTestValue1 = "defdata";
const std::string kTestKey2 = "123";
const std::string kTestValue2 = "123data";
const std::string kTestCopyPrefix1 = "www";
const std::string kTestCopyPrefix2 = "xxx";
const std::string kTestCopyPrefix3 = "yyy";
const std::string kTestSource = "source";
const size_t kTestSizeLimit = 512;

const std::vector<uint8_t> kTestPrefixVec = StrToVec(kTestPrefix);
const std::vector<uint8_t> kTestKey1Vec = StrToVec(kTestKey1);
const std::vector<uint8_t> kTestValue1Vec = StrToVec(kTestValue1);
const std::vector<uint8_t> kTestKey2Vec = StrToVec(kTestKey2);
const std::vector<uint8_t> kTestValue2Vec = StrToVec(kTestValue2);

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
    if (closure_)
      std::move(closure_).Run();
  }

  bool* result_;
  base::OnceClosure closure_;
};

class MockDelegate : public LevelDBWrapperImpl::Delegate {
 public:
  MockDelegate() {}
  ~MockDelegate() override {}

  void OnNoBindings() override {}
  std::vector<leveldb::mojom::BatchedOperationPtr> PrepareToCommit() override {
    return std::vector<leveldb::mojom::BatchedOperationPtr>();
  }
  void DidCommit(leveldb::mojom::DatabaseError error) override {
    if (error != leveldb::mojom::DatabaseError::OK)
      LOG(ERROR) << "error committing!";
    if (committed_)
      std::move(committed_).Run();
  }
  void OnMapLoaded(leveldb::mojom::DatabaseError error) override {
    map_load_count_++;
  }
  std::vector<LevelDBWrapperImpl::Change> FixUpData(
      const LevelDBWrapperImpl::ValueMap& data) override {
    return std::move(mock_changes_);
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

LevelDBWrapperImpl::Options GetDefaultTestingOptions(CacheMode cache_mode) {
  LevelDBWrapperImpl::Options options;
  options.max_size = kTestSizeLimit;
  options.default_commit_delay = base::TimeDelta::FromSeconds(5);
  options.max_bytes_per_hour = 10 * 1024 * 1024;
  options.max_commits_per_hour = 60;
  options.cache_mode = cache_mode;
  return options;
}

}  // namespace

class LevelDBWrapperImplTest : public testing::Test,
                               public mojom::LevelDBObserver {
 public:
  struct Observation {
    enum { kAdd, kChange, kDelete, kDeleteAll, kSendOldValue } type;
    std::string key;
    std::string old_value;
    std::string new_value;
    std::string source;
    bool should_send_old_value;
  };

  LevelDBWrapperImplTest() : db_(&mock_data_), observer_binding_(this) {
    auto request = mojo::MakeRequest(&level_db_database_ptr_);
    db_.Bind(std::move(request));

    LevelDBWrapperImpl::Options options =
        GetDefaultTestingOptions(CacheMode::KEYS_ONLY_WHEN_POSSIBLE);
    level_db_wrapper_ = std::make_unique<LevelDBWrapperImpl>(
        level_db_database_ptr_.get(), kTestPrefix, &delegate_, options);

    set_mock_data(kTestPrefix + kTestKey1, kTestValue1);
    set_mock_data(kTestPrefix + kTestKey2, kTestValue2);
    set_mock_data("123", "baddata");

    level_db_wrapper_->Bind(mojo::MakeRequest(&level_db_wrapper_ptr_));
    mojom::LevelDBObserverAssociatedPtrInfo ptr_info;
    observer_binding_.Bind(mojo::MakeRequest(&ptr_info));
    level_db_wrapper_ptr_->AddObserver(std::move(ptr_info));
  }

  ~LevelDBWrapperImplTest() override {}

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

  std::string GetSyncStrForKeyOnly(LevelDBWrapperImpl* wrapper_impl,
                                   const std::string& key) {
    leveldb::mojom::DatabaseError status;
    std::vector<mojom::KeyValuePtr> data;
    bool done = false;

    base::RunLoop loop;
    // Testing the 'Sync' mojo version is a big headache involving 3 threads, so
    // just test the async version.
    wrapper_impl->GetAll(
        GetAllCallback::CreateAndBind(&done, loop.QuitClosure()),
        base::BindOnce(&GetAllDataCallback, &status, &data));
    loop.Run();

    if (!done)
      return "";

    if (status != leveldb::mojom::DatabaseError::OK)
      return "";

    for (const auto& key_value : data) {
      if (key_value->key == StrToVec(key)) {
        return VecToStr(key_value->value);
      }
    }
    return "";
  }

  void ScheduleForkToPrefix(LevelDBWrapperImpl* source,
                            const std::string& new_prefix,
                            LevelDBWrapperImpl::Delegate* delegate,
                            LevelDBWrapperImpl::Options options,
                            std::unique_ptr<LevelDBWrapperImpl>* destination,
                            base::OnceClosure done) {
    base::PostTaskAndReplyWithResult(
        base::SequencedTaskRunnerHandle::Get().get(), FROM_HERE,
        base::BindOnce(&LevelDBWrapperImpl::ForkToNewPrefix,
                       base::Unretained(source), new_prefix, delegate, options),
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

  void should_record_send_old_value_observations(bool value) {
    should_record_send_old_value_observations_ = value;
  }

 private:
  // LevelDBObserver:
  void KeyAdded(const std::vector<uint8_t>& key,
                const std::vector<uint8_t>& value,
                const std::string& source) override {
    observations_.push_back(
        {Observation::kAdd, VecToStr(key), "", VecToStr(value), source, false});
  }
  void KeyChanged(const std::vector<uint8_t>& key,
                  const std::vector<uint8_t>& new_value,
                  const std::vector<uint8_t>& old_value,
                  const std::string& source) override {
    observations_.push_back({Observation::kChange, VecToStr(key),
                             VecToStr(old_value), VecToStr(new_value), source,
                             false});
  }
  void KeyDeleted(const std::vector<uint8_t>& key,
                  const std::vector<uint8_t>& old_value,
                  const std::string& source) override {
    observations_.push_back({Observation::kDelete, VecToStr(key),
                             VecToStr(old_value), "", source, false});
  }
  void AllDeleted(const std::string& source) override {
    observations_.push_back(
        {Observation::kDeleteAll, "", "", "", source, false});
  }
  void ShouldSendOldValueOnMutations(bool value) override {
    if (should_record_send_old_value_observations_)
      observations_.push_back(
          {Observation::kSendOldValue, "", "", "", "", value});
  }

  TestBrowserThreadBundle thread_bundle_;
  std::map<std::vector<uint8_t>, std::vector<uint8_t>> mock_data_;
  FakeLevelDBDatabase db_;
  leveldb::mojom::LevelDBDatabasePtr level_db_database_ptr_;
  MockDelegate delegate_;
  std::unique_ptr<LevelDBWrapperImpl> level_db_wrapper_;
  mojom::LevelDBWrapperPtr level_db_wrapper_ptr_;
  mojo::AssociatedBinding<mojom::LevelDBObserver> observer_binding_;
  std::vector<Observation> observations_;
  bool should_record_send_old_value_observations_ = false;
};

class ParamTest : public LevelDBWrapperImplTest,
                  public testing::WithParamInterface<CacheMode> {
 public:
  ParamTest() {}
  ~ParamTest() override {}
};

INSTANTIATE_TEST_CASE_P(LevelDBWrapperImplTest,
                        ParamTest,
                        testing::Values(CacheMode::KEYS_ONLY_WHEN_POSSIBLE,
                                        CacheMode::KEYS_AND_VALUES));

TEST_F(LevelDBWrapperImplTest, GetLoadedFromMap) {
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);
  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(kTestKey2Vec, &result));
  EXPECT_EQ(kTestValue2Vec, result);

  EXPECT_FALSE(GetSync(StrToVec("x"), &result));
}

TEST_F(LevelDBWrapperImplTest, GetFromPutOverwrite) {
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);
  std::vector<uint8_t> key = kTestKey2Vec;
  std::vector<uint8_t> value = StrToVec("foo");

  base::RunLoop loop;
  bool put_success = false;
  base::RepeatingClosure barrier = base::BarrierClosure(2, loop.QuitClosure());
  Put(key, value, kTestValue2Vec, &put_success, barrier);

  std::vector<uint8_t> result;
  bool get_success = false;
  Get(key, &get_success, &result, barrier);

  loop.Run();
  EXPECT_TRUE(put_success);
  EXPECT_TRUE(get_success);

  EXPECT_EQ(value, result);
}

TEST_F(LevelDBWrapperImplTest, GetFromPutNewKey) {
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);
  std::vector<uint8_t> key = StrToVec("newkey");
  std::vector<uint8_t> value = StrToVec("foo");

  EXPECT_TRUE(PutSync(key, value, base::nullopt));

  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(key, &result));
  EXPECT_EQ(value, result);
}

TEST_P(ParamTest, GetAll) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, CommitPutToDB) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::string value1 = "foo";
  std::string key2 = "abc";
  std::string value2 = "data abc";

  base::RunLoop loop;
  bool put_success1 = false;
  bool put_success2 = false;
  bool put_success3 = false;
  base::RepeatingClosure barrier = base::BarrierClosure(3, loop.QuitClosure());

  Put(kTestKey2Vec, StrToVec(value1), kTestValue2Vec, &put_success1, barrier);
  Put(StrToVec(key2), StrToVec("old value"), base::nullopt, &put_success2,
      barrier);
  Put(StrToVec(key2), StrToVec(value2), StrToVec("old value"), &put_success3,
      barrier);

  loop.Run();
  EXPECT_TRUE(put_success1 && put_success2 && put_success3);

  EXPECT_FALSE(has_mock_data(kTestPrefix + key2));

  CommitChanges();

  EXPECT_TRUE(has_mock_data(kTestPrefix + kTestKey2));
  EXPECT_EQ(value1, get_mock_data(kTestPrefix + kTestKey2));
  EXPECT_TRUE(has_mock_data(kTestPrefix + key2));
  EXPECT_EQ(value2, get_mock_data(kTestPrefix + key2));
}

TEST_P(ParamTest, PutObservations) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, DeleteNonExistingKey) {
  EXPECT_TRUE(DeleteSync(StrToVec("doesn't exist"), std::vector<uint8_t>()));
  EXPECT_EQ(0u, observations().size());
}

TEST_P(ParamTest, DeleteExistingKey) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, DeleteAllWithoutLoadedMap) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, DeleteAllWithLoadedMap) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, DeleteAllWithPendingMapLoad) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, DeleteAllWithoutLoadedEmptyMap) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  clear_mock_data();

  EXPECT_TRUE(DeleteAllSync());
  ASSERT_EQ(0u, observations().size());
}

TEST_P(ParamTest, PutOverQuotaLargeValue) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::vector<uint8_t> key = StrToVec("newkey");
  std::vector<uint8_t> value(kTestSizeLimit, 4);

  EXPECT_FALSE(PutSync(key, value, base::nullopt));

  value.resize(kTestSizeLimit / 2);
  EXPECT_TRUE(PutSync(key, value, base::nullopt));
}

TEST_P(ParamTest, PutOverQuotaLargeKey) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::vector<uint8_t> key(kTestSizeLimit, 'a');
  std::vector<uint8_t> value = StrToVec("newvalue");

  EXPECT_FALSE(PutSync(key, value, base::nullopt));

  key.resize(kTestSizeLimit / 2);
  EXPECT_TRUE(PutSync(key, value, base::nullopt));
}

TEST_P(ParamTest, PutWhenAlreadyOverQuota) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, PutWhenAlreadyOverQuotaBecauseOfLargeKey) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
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

TEST_P(ParamTest, PutAfterPurgeMemory) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::vector<uint8_t> result;
  const auto key = kTestKey2Vec;
  const auto value = kTestValue2Vec;
  EXPECT_TRUE(PutSync(key, value, value));
  EXPECT_EQ(delegate()->map_load_count(), 1);

  // Adding again doesn't load map again.
  EXPECT_TRUE(PutSync(key, value, value));
  EXPECT_EQ(delegate()->map_load_count(), 1);

  wrapper_impl()->PurgeMemory();

  // Now adding should still work, and load map again.
  EXPECT_TRUE(PutSync(key, value, value));
  EXPECT_EQ(delegate()->map_load_count(), 2);
}

TEST_P(ParamTest, PurgeMemoryWithPendingChanges) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::vector<uint8_t> key = kTestValue2Vec;
  std::vector<uint8_t> value = StrToVec("foo");
  EXPECT_TRUE(PutSync(key, value, kTestValue2Vec));
  EXPECT_EQ(delegate()->map_load_count(), 1);

  // Purge memory, and read. Should not actually have purged, so should not have
  // triggered a load.
  wrapper_impl()->PurgeMemory();

  EXPECT_TRUE(PutSync(key, value, value));
  EXPECT_EQ(delegate()->map_load_count(), 1);
}

TEST_P(ParamTest, FixUpData) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::vector<LevelDBWrapperImpl::Change> changes;
  changes.push_back(std::make_pair(StrToVec("def"), StrToVec("foo")));
  changes.push_back(std::make_pair(StrToVec("123"), base::nullopt));
  changes.push_back(std::make_pair(StrToVec("abc"), StrToVec("bla")));
  delegate()->set_mock_changes(std::move(changes));

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
  ASSERT_EQ(2u, data.size());
  EXPECT_EQ("abc", VecToStr(data[0]->key));
  EXPECT_EQ("bla", VecToStr(data[0]->value));
  EXPECT_EQ("def", VecToStr(data[1]->key));
  EXPECT_EQ("foo", VecToStr(data[1]->value));

  EXPECT_FALSE(has_mock_data(kTestPrefix + std::string("123")));
  EXPECT_EQ("foo", get_mock_data(kTestPrefix + std::string("def")));
  EXPECT_EQ("bla", get_mock_data(kTestPrefix + std::string("abc")));
}

TEST_F(LevelDBWrapperImplTest, SetOnlyKeysWithoutDatabase) {
  std::vector<uint8_t> key = kTestKey2Vec;
  std::vector<uint8_t> value = StrToVec("foo");
  MockDelegate delegate;
  LevelDBWrapperImpl level_db_wrapper(
      nullptr, kTestPrefix, &delegate,
      GetDefaultTestingOptions(CacheMode::KEYS_ONLY_WHEN_POSSIBLE));
  mojom::LevelDBWrapperPtr level_db_wrapper_ptr;
  level_db_wrapper.Bind(mojo::MakeRequest(&level_db_wrapper_ptr));
  // Setting only keys mode is noop.
  level_db_wrapper.SetCacheModeForTesting(CacheMode::KEYS_ONLY_WHEN_POSSIBLE);

  EXPECT_FALSE(level_db_wrapper.initialized());
  EXPECT_EQ(CacheMode::KEYS_AND_VALUES, level_db_wrapper.cache_mode());

  // Put and Get can work synchronously without reload.
  bool put_callback_called = false;
  level_db_wrapper.Put(key, value, base::nullopt, "source",
                       base::BindOnce(
                           [](bool* put_callback_called, bool success) {
                             EXPECT_TRUE(success);
                             *put_callback_called = true;
                           },
                           &put_callback_called));
  EXPECT_TRUE(put_callback_called);

  std::vector<uint8_t> expected_value;
  level_db_wrapper.Get(
      key, base::BindOnce(
               [](std::vector<uint8_t>* expected_value, bool success,
                  const std::vector<uint8_t>& value) {
                 EXPECT_TRUE(success);
                 *expected_value = value;
               },
               &expected_value));
  EXPECT_EQ(expected_value, value);
}

TEST_P(ParamTest, CommitOnDifferentCacheModes) {
  wrapper_impl()->SetCacheModeForTesting(GetParam());
  std::vector<uint8_t> key = kTestKey2Vec;
  std::vector<uint8_t> value = StrToVec("foo");
  std::vector<uint8_t> value2 = StrToVec("foobar");

  // The initial map always has values, so a nullopt is fine for the old value.
  ASSERT_TRUE(PutSync(key, value, base::nullopt));
  ASSERT_TRUE(wrapper_impl()->commit_batch_);
  auto* changes = &wrapper_impl()->commit_batch_->changed_values;
  EXPECT_EQ(1u, changes->size());
  auto it = changes->find(key);
  ASSERT_NE(it, changes->end());
  EXPECT_FALSE(it->second);

  CommitChanges();

  EXPECT_EQ("foo", get_mock_data(kTestPrefix + kTestKey2));
  if (GetParam() == CacheMode::KEYS_AND_VALUES)
    EXPECT_EQ(2u, wrapper_impl()->keys_values_map_.size());
  else
    EXPECT_EQ(2u, wrapper_impl()->keys_only_map_.size());
  ASSERT_TRUE(PutSync(key, value, value));
  EXPECT_FALSE(wrapper_impl()->commit_batch_);
  ASSERT_TRUE(PutSync(key, value2, value));
  ASSERT_TRUE(wrapper_impl()->commit_batch_);
  changes = &wrapper_impl()->commit_batch_->changed_values;
  EXPECT_EQ(1u, changes->size());
  it = changes->find(key);
  ASSERT_NE(it, changes->end());
  if (GetParam() == CacheMode::KEYS_AND_VALUES)
    EXPECT_FALSE(it->second);
  else
    EXPECT_EQ(value2, it->second.value());

  clear_mock_data();
  CommitChanges();
  EXPECT_EQ("foobar", get_mock_data(kTestPrefix + kTestKey2));
}

TEST_F(LevelDBWrapperImplTest, GetAllWhenCacheOnlyKeys) {
  std::vector<uint8_t> key = kTestKey2Vec;
  std::vector<uint8_t> value = StrToVec("foo");
  std::vector<uint8_t> value2 = StrToVec("foobar");

  // Go to load state only keys.
  ASSERT_TRUE(PutSync(key, value, base::nullopt));
  CommitChanges();
  ASSERT_TRUE(PutSync(key, value2, value));
  EXPECT_TRUE(wrapper_impl()->has_changes_to_commit());

  leveldb::mojom::DatabaseError status;
  std::vector<mojom::KeyValuePtr> data;
  bool result = false;

  base::RunLoop loop;

  bool put_result1 = false;
  bool put_result2 = false;
  {
    IncrementalBarrier barrier(loop.QuitClosure());

    Put(wrapper_impl(), key, value, value2, &put_result1, barrier.Get());

    wrapper_impl()->GetAll(
        GetAllCallback::CreateAndBind(&result, barrier.Get()),
        base::BindOnce(&GetAllDataCallback, &status, &data));
    Put(wrapper_impl(), key, value2, value, &put_result2, barrier.Get());
  }

  // GetAll triggers a commit when it's switching map types.
  EXPECT_TRUE(put_result1);
  EXPECT_EQ("foo", get_mock_data(kTestPrefix + kTestKey2));

  EXPECT_FALSE(put_result2);
  EXPECT_FALSE(result);
  loop.Run();

  EXPECT_TRUE(result && put_result1);

  EXPECT_EQ(2u, data.size());
  EXPECT_TRUE(data[1]->Equals(mojom::KeyValue(kTestKey1Vec, kTestValue1Vec)))
      << VecToStr(data[1]->value) << " vs expected " << kTestValue1;
  EXPECT_TRUE(data[0]->Equals(mojom::KeyValue(key, value)))
      << VecToStr(data[0]->value) << " vs expected " << VecToStr(value);

  EXPECT_EQ(leveldb::mojom::DatabaseError::OK, status);

  // The last "put" isn't committed yet.
  EXPECT_EQ("foo", get_mock_data(kTestPrefix + kTestKey2));

  ASSERT_TRUE(wrapper_impl()->has_changes_to_commit());
  CommitChanges();

  EXPECT_EQ("foobar", get_mock_data(kTestPrefix + kTestKey2));
}

TEST_F(LevelDBWrapperImplTest, GetAllAfterSetCacheMode) {
  std::vector<uint8_t> key = kTestKey2Vec;
  std::vector<uint8_t> value = StrToVec("foo");
  std::vector<uint8_t> value2 = StrToVec("foobar");

  // Go to load state only keys.
  ASSERT_TRUE(PutSync(key, value, base::nullopt));
  CommitChanges();
  EXPECT_TRUE(wrapper_impl()->map_state_ ==
              LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY);
  ASSERT_TRUE(PutSync(key, value2, value));
  EXPECT_TRUE(wrapper_impl()->has_changes_to_commit());

  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);

  // Cache isn't cleared when commit batch exists.
  EXPECT_TRUE(wrapper_impl()->map_state_ ==
              LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY);

  base::RunLoop loop;

  bool put_success = false;
  leveldb::mojom::DatabaseError status;
  std::vector<mojom::KeyValuePtr> data;
  bool get_all_success = false;
  bool delete_success = false;
  {
    IncrementalBarrier barrier(loop.QuitClosure());

    Put(wrapper_impl(), key, value, value2, &put_success, barrier.Get());

    // Commit batch still has old value. New Put() task is queued.
    ASSERT_TRUE(wrapper_impl()->has_changes_to_commit());
    EXPECT_EQ(value,
              wrapper_impl()->commit_batch_->changed_values.find(key)->second);

    wrapper_impl()->GetAll(
        GetAllCallback::CreateAndBind(&get_all_success, barrier.Get()),
        base::BindOnce(&GetAllDataCallback, &status, &data));

    // This Delete() should not affect the value returned by GetAll().
    Delete(wrapper_impl(), key, value, &delete_success, barrier.Get());
  }
  loop.Run();

  EXPECT_EQ(2u, data.size());
  EXPECT_TRUE(data[1]->Equals(mojom::KeyValue(kTestKey1Vec, kTestValue1Vec)))
      << VecToStr(data[1]->value) << " vs expected " << kTestValue1;
  EXPECT_TRUE(data[0]->Equals(mojom::KeyValue(key, value)))
      << VecToStr(data[0]->value) << " vs expected " << VecToStr(value2);

  EXPECT_EQ(leveldb::mojom::DatabaseError::OK, status);

  EXPECT_TRUE(put_success && get_all_success && delete_success);

  // GetAll shouldn't trigger a commit before it runs now because the value
  // map should be loading.
  EXPECT_EQ("foo", get_mock_data(kTestPrefix + kTestKey2));

  ASSERT_TRUE(wrapper_impl()->has_changes_to_commit());
  CommitChanges();

  EXPECT_FALSE(has_mock_data(kTestPrefix + kTestKey2));
}

TEST_F(LevelDBWrapperImplTest, SetCacheModeConsistent) {
  std::vector<uint8_t> key = kTestKey2Vec;
  std::vector<uint8_t> value = StrToVec("foo");
  std::vector<uint8_t> value2 = StrToVec("foobar");

  EXPECT_FALSE(wrapper_impl()->IsMapLoaded());
  EXPECT_TRUE(wrapper_impl()->cache_mode() ==
              CacheMode::KEYS_ONLY_WHEN_POSSIBLE);

  // Clear the database before the wrapper loads data.
  clear_mock_data();

  EXPECT_TRUE(PutSync(key, value, base::nullopt));
  EXPECT_TRUE(wrapper_impl()->has_changes_to_commit());
  CommitChanges();

  EXPECT_TRUE(PutSync(key, value2, value));
  EXPECT_TRUE(wrapper_impl()->has_changes_to_commit());

  // Setting cache mode does not reload the cache till it is required.
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY,
            wrapper_impl()->map_state_);

  // More put operations should stay in the same mode.
  EXPECT_TRUE(PutSync(key, value, value2));
  EXPECT_TRUE(wrapper_impl()->has_changes_to_commit());
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY,
            wrapper_impl()->map_state_);

  CommitChanges();

  // Still keys only. Calling a Get will load the map.
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY,
            wrapper_impl()->map_state_);
  std::vector<uint8_t> result;
  EXPECT_TRUE(GetSync(key, &result));
  EXPECT_EQ(value, result);
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_AND_VALUES,
            wrapper_impl()->map_state_);

  // Test that the map will unload only when there are no pending changes.
  // Add a pending change.
  EXPECT_TRUE(PutSync(key, value2, value));
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_ONLY_WHEN_POSSIBLE);
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_AND_VALUES,
            wrapper_impl()->map_state_);
  CommitChanges();
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY,
            wrapper_impl()->map_state_);

  // Test the map will unload right away when there are no changes.
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);
  EXPECT_TRUE(GetSync(key, &result));
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_AND_VALUES,
            wrapper_impl()->map_state_);
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_ONLY_WHEN_POSSIBLE);
  EXPECT_EQ(LevelDBWrapperImpl::MapState::LOADED_KEYS_ONLY,
            wrapper_impl()->map_state_);
}

TEST_F(LevelDBWrapperImplTest, SendOldValueObservations) {
  should_record_send_old_value_observations(true);
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_AND_VALUES);
  // Flush tasks on mojo thread to observe callback.
  EXPECT_TRUE(DeleteSync(StrToVec("doesn't exist"), base::nullopt));
  wrapper_impl()->SetCacheModeForTesting(CacheMode::KEYS_ONLY_WHEN_POSSIBLE);
  // Flush tasks on mojo thread to observe callback.
  EXPECT_TRUE(DeleteSync(StrToVec("doesn't exist"), base::nullopt));

  ASSERT_EQ(2u, observations().size());
  EXPECT_EQ(Observation::kSendOldValue, observations()[0].type);
  EXPECT_FALSE(observations()[0].should_send_old_value);
  EXPECT_EQ(Observation::kSendOldValue, observations()[1].type);
  EXPECT_TRUE(observations()[1].should_send_old_value);
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

  auto options = GetDefaultTestingOptions(CacheMode::KEYS_ONLY_WHEN_POSSIBLE);
  options.cache_mode = CacheMode::KEYS_AND_VALUES;
  {
    bool put_success1 = false;
    bool put_success2 = false;
    bool put_success3 = false;
    base::RunLoop loop;
    base::RepeatingClosure barrier =
        base::BarrierClosure(4, loop.QuitClosure());

    // Create fork 1.
    fork1 = wrapper_impl()->ForkToNewPrefix(kTestCopyPrefix1, &fork1_delegate,
                                            options);

    // Do a put on fork 1 and create fork 2.
    // Note - these are 'skipping' the mojo layer, which is why the fork isn't
    // scheduled.
    Put(fork1.get(), kTestKey2Vec, StrToVec(value4), kTestValue2Vec,
        &put_success1, barrier);
    fork2 = fork1->ForkToNewPrefix(kTestCopyPrefix2, &fork2_delegate, options);
    Put(fork1.get(), kTestKey2Vec, StrToVec(value5), StrToVec(value4),
        &put_success2, barrier);

    // Do a put on original and create fork 3, which is key-only.
    Put(kTestKey1Vec, StrToVec(value3), kTestValue1Vec, &put_success3, barrier);
    ScheduleForkToPrefix(
        wrapper_impl(), kTestCopyPrefix3, &fork3_delegate,
        GetDefaultTestingOptions(CacheMode::KEYS_ONLY_WHEN_POSSIBLE), &fork3,
        barrier);

    loop.Run();
    EXPECT_TRUE(put_success1);
    EXPECT_TRUE(put_success2);
    EXPECT_TRUE(fork2.get());
    EXPECT_TRUE(fork3.get());
  }

  EXPECT_EQ(value3, GetSyncStrForKeyOnly(wrapper_impl(), kTestKey1));
  EXPECT_EQ(kTestValue1, GetStrSync(fork1.get(), kTestKey1));
  EXPECT_EQ(kTestValue1, GetStrSync(fork2.get(), kTestKey1));
  EXPECT_EQ(value3, GetSyncStrForKeyOnly(fork3.get(), kTestKey1));

  EXPECT_EQ(kTestValue2, GetSyncStrForKeyOnly(wrapper_impl(), kTestKey2));
  EXPECT_EQ(value5, GetStrSync(fork1.get(), kTestKey2));
  EXPECT_EQ(value4, GetStrSync(fork2.get(), kTestKey2));
  EXPECT_EQ(kTestValue2, GetSyncStrForKeyOnly(fork3.get(), kTestKey2));

  ScheduleCommitChanges(delegate(), wrapper_impl());
  ScheduleCommitChanges(&fork1_delegate, fork1.get());

  // kTestKey1 values.
  EXPECT_EQ(value3, get_mock_data(kTestPrefix + kTestKey1));
  EXPECT_EQ(kTestValue1, get_mock_data(kTestCopyPrefix1 + kTestKey1));
  EXPECT_EQ(kTestValue1, get_mock_data(kTestCopyPrefix2 + kTestKey1));
  EXPECT_EQ(value3, get_mock_data(kTestCopyPrefix3 + kTestKey1));

  // kTestKey2 values.
  EXPECT_EQ(kTestValue2, get_mock_data(kTestPrefix + kTestKey2));
  EXPECT_EQ(value5, get_mock_data(kTestCopyPrefix1 + kTestKey2));
  EXPECT_EQ(value4, get_mock_data(kTestCopyPrefix2 + kTestKey2));
  EXPECT_EQ(kTestValue2, get_mock_data(kTestCopyPrefix3 + kTestKey2));
}

TEST_P(ParamTest, PrefixForkAfterLoad) {
  const std::string kValue = "foo";
  const std::vector<uint8_t> kValueVec = StrToVec(kValue);

  // Do a sync put so the map loads.
  EXPECT_TRUE(PutSync(kTestKey1Vec, kValueVec, base::nullopt));

  // Execute the fork.
  MockDelegate fork1_delegate;
  std::unique_ptr<LevelDBWrapperImpl> fork1 = wrapper_impl()->ForkToNewPrefix(
      kTestCopyPrefix1, &fork1_delegate, GetDefaultTestingOptions(GetParam()));

  // Check our forked state.
  EXPECT_EQ(kValue, GetSyncStrForKeyOnly(fork1.get(), kTestKey1));

  ScheduleCommitChanges(delegate(), wrapper_impl());

  EXPECT_EQ(kValue, get_mock_data(kTestCopyPrefix1 + kTestKey1));
}

std::string GetNewPrefix(int64_t* i) {
  std::string prefix = "prefix-" + base::Int64ToString(*i) + "-";
  (*i)++;
  return prefix;
}

std::vector<uint8_t> VecWithElement(uint8_t element) {
  std::vector<uint8_t> vec = {element};
  return vec;
}

base::Optional<std::vector<uint8_t>> EvaluatePastValue(bool exists,
                                                       uint8_t value) {
  return exists ? base::Optional<std::vector<uint8_t>>(VecWithElement(value))
                : base::nullopt;
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
        wrappers[i] = wrapper_impl()->ForkToNewPrefix(
            GetNewPrefix(&curr_prefix), &delegates[i],
            GetDefaultTestingOptions(CacheMode::KEYS_ONLY_WHEN_POSSIBLE));
      }
      int64_t forks = i;
      if (i % 5 == 0 || i % 6 == 0) {
        forks++;
        states[forks] = state;
        wrappers[forks] = wrappers[i]->ForkToNewPrefix(
            GetNewPrefix(&curr_prefix), &delegates[forks],
            GetDefaultTestingOptions(CacheMode::KEYS_AND_VALUES));
      }
      if (i % 13 == 0) {
        FuzzState old_state = state;
        state.has_val1 = false;
        successes.push_back(false);
        Delete(wrappers[i].get(), kKey1Vec,
               EvaluatePastValue(old_state.has_val1, old_state.val1),
               &successes.back(), barrier.Get());
      }
      if (i % 4 == 0) {
        FuzzState old_state = state;
        state.val2 = i;
        state.has_val2 = true;
        successes.push_back(false);
        Put(wrappers[i].get(), kKey2Vec, {state.val2},
            EvaluatePastValue(old_state.has_val2, old_state.val2),
            &successes.back(), barrier.Get());
      }
      if (i % 3 == 0) {
        FuzzState old_state = state;
        state.val1 = i + 5;
        state.has_val1 = true;
        successes.push_back(false);
        Put(wrappers[i].get(), kKey1Vec, {state.val1},
            EvaluatePastValue(old_state.has_val1, old_state.val1),
            &successes.back(), barrier.Get());
      }
      if (i % 11 == 0) {
        state.has_val1 = false;
        state.has_val2 = false;
        successes.push_back(false);
        DeleteAll(wrappers[i].get(), &successes.back(), barrier.Get());
      }
      if (i % 2 == 0) {
        CacheMode mode = i % 3 == 0 ? CacheMode::KEYS_AND_VALUES
                                    : CacheMode::KEYS_ONLY_WHEN_POSSIBLE;
        forks++;
        states[forks] = state;
        wrappers[forks] = wrappers[i]->ForkToNewPrefix(
            GetNewPrefix(&curr_prefix), &delegates[forks],
            GetDefaultTestingOptions(mode));
      }
      if (i % 3 == 0) {
        FuzzState old_state = state;
        state.val1 = i + 9;
        state.has_val1 = true;
        successes.push_back(false);
        Put(wrappers[i].get(), kKey1Vec, {state.val1},
            EvaluatePastValue(old_state.has_val1, old_state.val1),
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

    std::string result1 = GetSyncStrForKeyOnly(wrappers[i].get(), kKey1);
    std::string result2 = GetSyncStrForKeyOnly(wrappers[i].get(), kKey2);
    EXPECT_EQ(state.has_val1, !result1.empty()) << i;
    if (state.has_val1)
      EXPECT_EQ(state.val1, StrToVec(result1)[0]);
    EXPECT_EQ(state.has_val2, !result2.empty()) << i;
    if (state.has_val2)
      EXPECT_EQ(state.val2, StrToVec(result2)[0]) << i;
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
