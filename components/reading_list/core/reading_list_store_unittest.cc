// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reading_list/core/reading_list_store.h"

#include <map>
#include <set>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/simple_test_clock.h"
#include "components/reading_list/core/reading_list_model_impl.h"
#include "components/sync/model/fake_model_type_change_processor.h"
#include "components/sync/model/model_type_store_test_util.h"
#include "components/sync/test/model_type_sync_bridge_test_template.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Tests that the transition from |entryA| to |entryB| is possible (|possible|
// is true) or not.
void ExpectAB(const sync_pb::ReadingListSpecifics& entryA,
              const sync_pb::ReadingListSpecifics& entryB,
              bool possible) {
  EXPECT_EQ(ReadingListStore::CompareEntriesForSync(entryA, entryB), possible);
  std::unique_ptr<ReadingListEntry> a =
      ReadingListEntry::FromReadingListSpecifics(entryA,
                                                 base::Time::FromTimeT(10));
  std::unique_ptr<ReadingListEntry> b =
      ReadingListEntry::FromReadingListSpecifics(entryB,
                                                 base::Time::FromTimeT(10));
  a->MergeWithEntry(*b);
  std::unique_ptr<sync_pb::ReadingListSpecifics> mergedEntry =
      a->AsReadingListSpecifics();
  if (possible) {
    // If transition is possible, the merge should be B.
    EXPECT_EQ(entryB.SerializeAsString(), mergedEntry->SerializeAsString());
  } else {
    // If transition is not possible, the transition shold be possible to the
    // merged state.
    EXPECT_TRUE(ReadingListStore::CompareEntriesForSync(entryA, *mergedEntry));
    EXPECT_TRUE(ReadingListStore::CompareEntriesForSync(entryB, *mergedEntry));
  }
}

struct ReadingListAddParams {
  GURL url;
  std::string title;
  bool is_read;
};

class ReadingListTester
    : public syncer::ModelTypeSyncBridgeTester<ReadingListAddParams,
                                               sync_pb::ReadingListSpecifics,
                                               ReadingListTester> {
 public:
  static void Restart(std::unique_ptr<ReadingListTester>* tester) {
    auto factory = (*tester)->change_processor_factory_;
    auto store = std::move((*tester)->store_);
    tester->reset();
    *tester = std::make_unique<ReadingListTester>(factory, std::move(store));
  }

  ReadingListTester(
      const syncer::ModelTypeSyncBridge::ChangeProcessorFactory&
          change_processor_factory,
      std::unique_ptr<syncer::ModelTypeStore> store =
          syncer::ModelTypeStoreTestUtil::CreateInMemoryStoreForTest())
      : change_processor_factory_(change_processor_factory),
        store_(std::move(store)) {
    CHECK(store_);
    auto clock = std::make_unique<base::SimpleTestClock>();
    clock_ = clock.get();
    auto reading_list_store = std::make_unique<ReadingListStore>(
        base::Bind(&syncer::ModelTypeStoreTestUtil::MoveStoreToCallback,
                   base::Passed(&store_)),
        change_processor_factory);
    reading_list_store_ = reading_list_store.get();
    model_ = std::make_unique<ReadingListModelImpl>(
        std::move(reading_list_store), /*pref_service=*/nullptr,
        std::move(clock));
  }

  ~ReadingListTester() override {}

  static syncer::EntityDataPtr SpecificsToEntityImpl(
      const SPECIFICS& specifics) {
    syncer::EntityData data;
    // TODO(mastiz): Revisit why this is needed to pass MergeWithoutLocal(),
    // where otherwise PassToPtr() below drops the specifics.
    data.client_tag_hash = "ignored";
    *data.specifics.mutable_reading_list() = specifics;
    return data.PassToPtr();
  }

  static const SPECIFICS& SelectSpecificsImpl(
      const sync_pb::EntitySpecifics& entity_specifics) {
    return entity_specifics.reading_list();
  }

  syncer::ModelTypeSyncBridge* bridge() override { return reading_list_store_; }

  TestEntityData GetTestEntity1() const override {
    int64_t time_us =
        (clock_->Now() - base::Time::UnixEpoch()).InMicroseconds();

    sync_pb::ReadingListSpecifics specifics;
    specifics.set_title("unread title");
    specifics.set_url("http://unread.example.com/");
    specifics.set_entry_id("http://unread.example.com/");
    specifics.set_status(sync_pb::ReadingListSpecifics::UNSEEN);
    specifics.set_creation_time_us(time_us);
    specifics.set_update_time_us(time_us);
    specifics.set_first_read_time_us(0);
    specifics.set_update_title_time_us(time_us);

    ReadingListAddParams native;
    native.url = GURL("http://unread.example.com/");
    native.title = "unread title";
    native.is_read = false;
    return {native, specifics};
  }

  TestEntityData GetTestEntity2() const override {
    int64_t time_us =
        (clock_->Now() - base::Time::UnixEpoch()).InMicroseconds();

    sync_pb::ReadingListSpecifics specifics;
    specifics.set_title("read title");
    specifics.set_url("http://read.example.com/");
    specifics.set_entry_id("http://read.example.com/");
    specifics.set_status(sync_pb::ReadingListSpecifics::READ);
    specifics.set_creation_time_us(time_us);
    specifics.set_update_time_us(time_us);
    specifics.set_first_read_time_us(time_us);
    specifics.set_update_title_time_us(time_us);

    ReadingListAddParams native;
    native.url = GURL("http://read.example.com/");
    native.title = "read title";
    native.is_read = true;
    return {native, specifics};
  }

  std::string StoreEntityInLocalModel(NATIVE_T* native) override {
    model_->AddEntry(native->url, native->title,
                     reading_list::ADDED_VIA_CURRENT_APP);
    model_->SetReadStatus(native->url, native->is_read);
    return native->url.spec();
  }

  void DeleteEntityFromLocalModel(const std::string& storage_key) override {
    model_->RemoveEntryByURL(GURL(storage_key));
  }

  SPECIFICS GetMutatedTestEntity1Specifics() const override {
    int64_t time_us =
        (clock_->Now() - base::Time::UnixEpoch()).InMicroseconds();
    SPECIFICS specifics = GetTestEntity1().specifics;
    specifics.set_status(sync_pb::ReadingListSpecifics::READ);
    specifics.set_first_read_time_us(time_us);
    return specifics;
  }

  void MutateStoredTestEntity1() override {
    model_->SetReadStatus(GetTestEntity1().native.url, true);
  }

 private:
  const syncer::ModelTypeSyncBridge::ChangeProcessorFactory
      change_processor_factory_;
  std::unique_ptr<syncer::ModelTypeStore> store_;
  std::unique_ptr<ReadingListModelImpl> model_;
  base::SimpleTestClock* clock_;
  ReadingListStore* reading_list_store_;
};

}  // namespace

/*
class ReadingListStoreTest : public testing::Test {
 protected:
  ReadingListStoreTest() :
tester_(base::Bind(&ReadingListStoreTest::CreateModelTypeChangeProcessor,
                   base::Unretained(this))) {
    base::RunLoop().RunUntilIdle();
  }

  std::unique_ptr<syncer::ModelTypeChangeProcessor>
  CreateModelTypeChangeProcessor(syncer::ModelType type,
                                 syncer::ModelTypeSyncBridge* service) {
    auto processor = base::MakeUnique<TestModelTypeChangeProcessor>();
    processor->SetObserver(this);
    return std::move(processor);
  }

  // In memory model type store needs a MessageLoop.
  base::MessageLoop message_loop_;
  ReadingListTester tester_;
};

TEST_F(ReadingListStoreTest, ApplySyncChangesOneMerge) {
  AdvanceAndGetTime(clock_);
  model_->AddEntry(GURL("http://unread.example.com/"), "unread title",
                   reading_list::ADDED_VIA_CURRENT_APP);

  ReadingListEntry new_entry(GURL("http://unread.example.com/"), "unread title",
                             AdvanceAndGetTime(clock_));
  new_entry.SetRead(true, AdvanceAndGetTime(clock_));
  std::unique_ptr<sync_pb::ReadingListSpecifics> specifics =
      new_entry.AsReadingListSpecifics();
  syncer::EntityData data;
  data.client_tag_hash = "http://unread.example.com/";
  *data.specifics.mutable_reading_list() = *specifics;

  syncer::EntityChangeList add_changes;
  add_changes.push_back(syncer::EntityChange::CreateAdd(
      "http://unread.example.com/", data.PassToPtr()));
  auto error = reading_list_store_->ApplySyncChanges(
      reading_list_store_->CreateMetadataChangeList(), add_changes);
  AssertCounts(1, 0, 0, 0, 1);
  EXPECT_EQ(sync_merged_.size(), 1u);
  EXPECT_EQ(sync_merged_.count("http://unread.example.com/"), 1u);
  EXPECT_EQ(sync_merged_["http://unread.example.com/"], true);
}

TEST_F(ReadingListStoreTest, ApplySyncChangesOneIgnored) {
  // Read entry but with unread URL as it must update the other one.
  ReadingListEntry old_entry(GURL("http://unread.example.com/"),
                             "old unread title", AdvanceAndGetTime(clock_));
  old_entry.SetRead(true, AdvanceAndGetTime(clock_));

  AdvanceAndGetTime(clock_);
  model_->AddEntry(GURL("http://unread.example.com/"), "new unread title",
                   reading_list::ADDED_VIA_CURRENT_APP);
  AssertCounts(0, 0, 0, 0, 0);

  std::unique_ptr<sync_pb::ReadingListSpecifics> specifics =
      old_entry.AsReadingListSpecifics();
  syncer::EntityData data;
  data.client_tag_hash = "http://unread.example.com/";
  *data.specifics.mutable_reading_list() = *specifics;

  syncer::EntityChangeList add_changes;
  add_changes.push_back(syncer::EntityChange::CreateAdd(
      "http://unread.example.com/", data.PassToPtr()));
  auto error = reading_list_store_->ApplySyncChanges(
      reading_list_store_->CreateMetadataChangeList(), add_changes);
  AssertCounts(1, 0, 0, 0, 1);
  EXPECT_EQ(sync_merged_.size(), 1u);
}

*/

TEST(ReadingListStoreTest, CompareEntriesForSync) {
  sync_pb::ReadingListSpecifics entryA;
  sync_pb::ReadingListSpecifics entryB;
  entryA.set_entry_id("http://foo.bar/");
  entryB.set_entry_id("http://foo.bar/");
  entryA.set_url("http://foo.bar/");
  entryB.set_url("http://foo.bar/");
  entryA.set_title("Foo Bar");
  entryB.set_title("Foo Bar");
  entryA.set_status(sync_pb::ReadingListSpecifics::UNREAD);
  entryB.set_status(sync_pb::ReadingListSpecifics::UNREAD);
  entryA.set_creation_time_us(10);
  entryB.set_creation_time_us(10);
  entryA.set_first_read_time_us(50);
  entryB.set_first_read_time_us(50);
  entryA.set_update_time_us(100);
  entryB.set_update_time_us(100);
  entryA.set_update_title_time_us(110);
  entryB.set_update_title_time_us(110);
  // Equal entries can be submitted.
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, true);

  // Try to update each field.

  // You cannot change the URL of an entry.
  entryA.set_url("http://foo.foo/");
  EXPECT_FALSE(ReadingListStore::CompareEntriesForSync(entryA, entryB));
  EXPECT_FALSE(ReadingListStore::CompareEntriesForSync(entryB, entryA));
  entryA.set_url("http://foo.bar/");

  // You can set a title to a title later in alphabetical order if the
  // update_title_time is the same. If a title has been more recently updated,
  // the only possible transition is to this one.
  entryA.set_title("");
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(109);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(110);

  entryA.set_title("Foo Aar");
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(109);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(110);

  entryA.set_title("Foo Ba");
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(109);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(110);

  entryA.set_title("Foo Bas");
  ExpectAB(entryA, entryB, false);
  ExpectAB(entryB, entryA, true);
  entryA.set_update_title_time_us(109);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_update_title_time_us(110);
  entryA.set_title("Foo Bar");

  // Update times.
  entryA.set_creation_time_us(9);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_first_read_time_us(51);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_first_read_time_us(49);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_first_read_time_us(0);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_first_read_time_us(50);
  entryB.set_first_read_time_us(0);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryB.set_first_read_time_us(50);
  entryA.set_creation_time_us(10);
  entryA.set_first_read_time_us(51);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_first_read_time_us(0);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  entryA.set_first_read_time_us(50);

  entryA.set_update_time_us(99);
  ExpectAB(entryA, entryB, true);
  ExpectAB(entryB, entryA, false);
  sync_pb::ReadingListSpecifics::ReadingListEntryStatus status_oder[3] = {
      sync_pb::ReadingListSpecifics::UNSEEN,
      sync_pb::ReadingListSpecifics::UNREAD,
      sync_pb::ReadingListSpecifics::READ};
  for (int index_a = 0; index_a < 3; index_a++) {
    entryA.set_status(status_oder[index_a]);
    for (int index_b = 0; index_b < 3; index_b++) {
      entryB.set_status(status_oder[index_b]);
      ExpectAB(entryA, entryB, true);
      ExpectAB(entryB, entryA, false);
    }
  }
  entryA.set_update_time_us(100);
  for (int index_a = 0; index_a < 3; index_a++) {
    entryA.set_status(status_oder[index_a]);
    entryB.set_status(status_oder[index_a]);
    ExpectAB(entryA, entryB, true);
    ExpectAB(entryB, entryA, true);
    for (int index_b = index_a + 1; index_b < 3; index_b++) {
      entryB.set_status(status_oder[index_b]);
      ExpectAB(entryA, entryB, true);
      ExpectAB(entryB, entryA, false);
    }
  }
}

namespace syncer {

INSTANTIATE_TYPED_TEST_CASE_P(ReadingList, SyncBridgeTest, ReadingListTester);

}  // namespace syncer
