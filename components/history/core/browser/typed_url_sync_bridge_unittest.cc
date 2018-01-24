// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/typed_url_sync_bridge.h"

#include <memory>

#include "base/big_endian.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/history/core/browser/history_backend.h"
#include "components/history/core/browser/history_backend_client.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/in_memory_history_backend.h"
#include "components/history/core/test/test_history_database.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/recording_model_type_change_processor.h"
#include "components/sync/model/sync_metadata_store.h"
#include "components/sync/test/model_type_sync_bridge_test_template.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_pb::EntitySpecifics;
using sync_pb::TypedUrlSpecifics;
using syncer::DataBatch;
using syncer::EntityChange;
using syncer::EntityChangeList;
using syncer::EntityData;
using syncer::EntityDataPtr;
using syncer::KeyAndData;
using syncer::MetadataBatch;
using syncer::MetadataChangeList;
using syncer::RecordingModelTypeChangeProcessor;

namespace history {

namespace {

// Constants used to limit size of visits processed. See
// equivalent constants in typed_url_sync_bridge.cc for descriptions.
const int kMaxTypedUrlVisits = 100;
const int kVisitThrottleThreshold = 10;
const int kVisitThrottleMultiple = 10;

// Visits with this timestamp are treated as expired.
const int kExpiredVisit = -1;

// Helper constants for tests.
const char kTitle[] = "pie";
const char kTitle2[] = "cookie";
const char kURLStr[] = "http://pie.com/";
const char kURLStr2[] = "http://cookie.com/";

bool URLsEqual(URLRow& row, TypedUrlSpecifics& specifics) {
  return ((row.url().spec().compare(specifics.url()) == 0) &&
          (base::UTF16ToUTF8(row.title()).compare(specifics.title()) == 0) &&
          (row.hidden() == specifics.hidden()));
}

bool URLsEqual(URLRow& lhs, URLRow& rhs) {
  // Only compare synced fields (ignore typed_count and visit_count as those
  // are maintained by the history subsystem).
  return (lhs.url().spec().compare(rhs.url().spec()) == 0) &&
         (lhs.title().compare(rhs.title()) == 0) &&
         (lhs.hidden() == rhs.hidden());
}

struct URLRowAndVisits : public syncer::ModelTypeSyncBridgeTester::LocalType {
  // Create a new row object and the typed visit çorresponding with the time at
  // |last_visit|.
  URLRowAndVisits(const GURL& url,
                  const std::string& title,
                  int typed_count,
                  int64_t last_visit,
                  bool hidden) {
    // Give each URL a unique ID, to mimic the behavior of the real database.
    row = URLRow(url);
    row.set_title(base::UTF8ToUTF16(title));
    row.set_typed_count(typed_count);

    base::Time last_visit_time = base::Time::FromInternalValue(last_visit);
    row.set_last_visit(last_visit_time);

    if (typed_count > 0) {
      // Add a typed visit for time |last_visit|.
      visits.push_back(
          VisitRow(row.id(), last_visit_time, 0, ui::PAGE_TRANSITION_TYPED, 0));
    } else {
      // Add a non-typed visit for time |last_visit|.
      visits.push_back(VisitRow(row.id(), last_visit_time, 0,
                                ui::PAGE_TRANSITION_RELOAD, 0));
    }

    row.set_visit_count(visits.size());
  }

  ~URLRowAndVisits() override = default;

  VisitRow MakeVisit(ui::PageTransition transition, int64_t visit_time) const {
    base::Time time = base::Time::FromInternalValue(visit_time);
    return VisitRow(row.id(), time, 0, transition, 0);
  }

  void AddNewestVisit(ui::PageTransition transition, int64_t visit_time) {
    visits.push_back(MakeVisit(transition, visit_time));
    if (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED)) {
      row.set_typed_count(row.typed_count() + 1);
    }
    row.set_last_visit(visits.front().visit_time);
    row.set_visit_count(visits.size());
  }

  void AddOldestVisit(ui::PageTransition transition, int64_t visit_time) {
    visits.insert(visits.begin(), MakeVisit(transition, visit_time));
    if (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED)) {
      row.set_typed_count(row.typed_count() + 1);
    }
    row.set_visit_count(visits.size());
  }

  URLRow row;
  VisitVector visits;
};

std::string IntToStorageKey(int id) {
  std::string storage_key(sizeof(URLID), 0);
  base::WriteBigEndian<URLID>(&storage_key[0], id);
  return storage_key;
}

class TestHistoryBackendDelegate : public HistoryBackend::Delegate {
 public:
  TestHistoryBackendDelegate() {}

  void NotifyProfileError(sql::InitStatus init_status,
                          const std::string& diagnostics) override {}
  void SetInMemoryBackend(
      std::unique_ptr<InMemoryHistoryBackend> backend) override {}
  void NotifyFaviconsChanged(const std::set<GURL>& page_urls,
                             const GURL& icon_url) override {}
  void NotifyURLVisited(ui::PageTransition transition,
                        const URLRow& row,
                        const RedirectList& redirects,
                        base::Time visit_time) override {}
  void NotifyURLsModified(const URLRows& changed_urls) override {}
  void NotifyURLsDeleted(bool all_history,
                         bool expired,
                         const URLRows& deleted_rows,
                         const std::set<GURL>& favicon_urls) override {}
  void NotifyKeywordSearchTermUpdated(const URLRow& row,
                                      KeywordID keyword_id,
                                      const base::string16& term) override {}
  void NotifyKeywordSearchTermDeleted(URLID url_id) override {}
  void DBLoaded() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestHistoryBackendDelegate);
};

class TestHistoryBackend : public HistoryBackend {
 public:
  TestHistoryBackend()
      : HistoryBackend(new TestHistoryBackendDelegate(),
                       nullptr,
                       base::ThreadTaskRunnerHandle::Get()) {}

  bool IsExpiredVisitTime(const base::Time& time) override {
    return time.ToInternalValue() == kExpiredVisit;
  }

  URLID GetIdByUrl(const GURL& gurl) {
    return db()->GetRowForURL(gurl, nullptr);
  }

  URLRowAndVisits AddRowAndVisits(const GURL& url,
                                  const std::string& title,
                                  int typed_count,
                                  int64_t last_visit,
                                  bool hidden) {
    URLRowAndVisits row_and_visits(url, title, typed_count, last_visit, hidden);
    AddRowAndVisits(&row_and_visits);
    return row_and_visits;
  }

  // Populates row ID in |row_and_visits->row|.
  void AddRowAndVisits(URLRowAndVisits* row_and_visits) {
    const GURL& url = row_and_visits->row.url();
    AddPagesWithDetails({row_and_visits->row}, SOURCE_SYNCED);
    row_and_visits->row.set_id(GetIdByUrl(url));
    AddVisitsForUrl(row_and_visits->row.url(), row_and_visits->visits);
  }

  void AddVisitsForUrl(const GURL& url, const VisitVector& visits) {
    std::vector<VisitInfo> added_visits;
    for (const auto& visit : visits) {
      added_visits.push_back(VisitInfo(visit.visit_time, visit.transition));
    }
    AddVisits(url, added_visits, SOURCE_SYNCED);
  }

 private:
  ~TestHistoryBackend() override {}
};

class TypedURLTester : public syncer::ModelTypeSyncBridgeTester {
 public:
  static std::unique_ptr<base::ScopedTempDir> CreateUniqueTempDir() {
    auto dir = std::make_unique<base::ScopedTempDir>();
    CHECK(dir->CreateUniqueTempDir());
    return dir;
  }

  TypedURLTester(
      const syncer::ModelTypeSyncBridge::ChangeProcessorFactory&
          change_processor_factory,
      std::unique_ptr<base::ScopedTempDir> test_dir = CreateUniqueTempDir())
      : change_processor_factory_(change_processor_factory),
        test_dir_(std::move(test_dir)),
        typed_url_sync_bridge_(nullptr) {
    fake_history_backend_ = new TestHistoryBackend();
    fake_history_backend_->Init(
        false, TestHistoryDatabaseParamsForPath(test_dir_->GetPath()));
    std::unique_ptr<TypedURLSyncBridge> bridge =
        std::make_unique<TypedURLSyncBridge>(fake_history_backend_.get(),
                                             fake_history_backend_->db(),
                                             change_processor_factory);
    typed_url_sync_bridge_ = bridge.get();
    typed_url_sync_bridge_->Init();
    fake_history_backend_->SetTypedURLSyncBridgeForTest(std::move(bridge));
  }

  ~TypedURLTester() override { fake_history_backend_->Closing(); }

  TypedURLSyncBridge* typed_url_sync_bridge() { return typed_url_sync_bridge_; }
  TestHistoryBackend* fake_history_backend() {
    return fake_history_backend_.get();
  }

  std::unique_ptr<ModelTypeSyncBridgeTester> MoveAndRestart() override {
    auto factory = change_processor_factory_;
    auto test_dir = std::move(test_dir_);
    return std::make_unique<TypedURLTester>(factory, std::move(test_dir));
  }

  syncer::ModelTypeSyncBridge* bridge() override {
    return typed_url_sync_bridge_;
  }

  EntitySpecifics GetTestSpecifics1() const {
    EntitySpecifics specifics;
    TypedUrlSpecifics* typed_url = specifics.mutable_typed_url();
    typed_url->set_url(kURLStr);
    typed_url->set_title(kTitle);
    typed_url->set_hidden(false);
    typed_url->add_visits(3);
    typed_url->add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
    return specifics;
  }

  TestEntityData GetTestEntity1() override {
    return TestEntityData(
        std::make_unique<URLRowAndVisits>(GURL(kURLStr), kTitle, 1, 3, false),
        GetTestSpecifics1());
  }

  TestEntityData GetTestEntity2() override {
    EntitySpecifics specifics;
    TypedUrlSpecifics* typed_url = specifics.mutable_typed_url();
    typed_url->set_url(kURLStr2);
    typed_url->set_title(kTitle2);
    typed_url->set_hidden(false);
    typed_url->add_visits(4);
    typed_url->add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
    return TestEntityData(
        std::make_unique<URLRowAndVisits>(GURL(kURLStr2), kTitle2, 1, 4, false),
        specifics);
  }

  std::string StoreEntityInLocalModel(LocalType* local) override {
    auto* typed_url = static_cast<URLRowAndVisits*>(local);
    fake_history_backend_->AddRowAndVisits(typed_url);
    return IntToStorageKey(typed_url->row.id());
  }

  void DeleteEntityFromLocalModel(const std::string& storage_key) override {
    URLID url_id = TypedURLSyncMetadataDatabase::StorageKeyToURLID(storage_key);
    URLRow row;
    ASSERT_TRUE(fake_history_backend_->GetURLByID(url_id, &row));
    fake_history_backend_->DeleteURL(row.url());
  }

  EntitySpecifics GetMutatedTestEntity1Specifics() override {
    EntitySpecifics specifics = GetTestSpecifics1();
    TypedUrlSpecifics* typed_url = specifics.mutable_typed_url();
    typed_url->add_visits(7);
    typed_url->add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
    return specifics;
  }

  void MutateStoredTestEntity1() override {
    VisitRow visit = static_cast<URLRowAndVisits*>(GetTestEntity1().local.get())
                         ->MakeVisit(ui::PAGE_TRANSITION_TYPED, 7);
    fake_history_backend_->AddVisitsForUrl(GURL(kURLStr), {visit});
  }

 private:
  syncer::ModelTypeSyncBridge::ChangeProcessorFactory change_processor_factory_;
  std::unique_ptr<base::ScopedTempDir> test_dir_;
  scoped_refptr<TestHistoryBackend> fake_history_backend_;
  TypedURLSyncBridge* typed_url_sync_bridge_;
};

}  // namespace

class TypedURLSyncBridgeTest : public testing::Test {
 public:
  const GURL kURL = GURL(kURLStr);
  const GURL kURL2 = GURL(kURLStr2);

  TypedURLSyncBridgeTest()
      : tester_(
            RecordingModelTypeChangeProcessor::FactoryForBridgeTest(&processor_,
                                                                    false)) {
    bridge()->history_backend_observer_.RemoveAll();
  }
  ~TypedURLSyncBridgeTest() override {}

  void TearDown() override { VerifyProcessorReceivedValidEntityData(); }

  // Starts sync for |typed_url_sync_bridge_| with |initial_data| as the
  // initial sync data.
  void StartSyncing(const std::vector<TypedUrlSpecifics>& specifics) {
    std::vector<EntitySpecifics> initial_data;
    for (const TypedUrlSpecifics& typed_url : specifics) {
      EntitySpecifics specifics;
      *specifics.mutable_typed_url() = typed_url;
      initial_data.push_back(specifics);
    }
    ASSERT_TRUE(false);
    // TODO tester_.StartSyncingAndWait(initial_data);
  }

  // TODO(mastiz): See if a vector of rows can be returned, if noone is
  // interested in visits.
  bool BuildAndPushLocalChanges(unsigned int num_typed_urls,
                                unsigned int num_reload_urls,
                                const std::vector<GURL>& urls,
                                std::vector<URLRowAndVisits>* rows_and_visits) {
    unsigned int total_urls = num_typed_urls + num_reload_urls;
    DCHECK(urls.size() >= total_urls);
    if (!bridge())
      return false;

    if (total_urls) {
      // Create new URL rows, populate the mock backend with its visits, and
      // send to the sync service.
      URLRows changed_urls;

      for (unsigned int i = 0; i < total_urls; ++i) {
        int typed = i < num_typed_urls ? 1 : 0;
        rows_and_visits->push_back(fake_history_backend()->AddRowAndVisits(
            urls[i], kTitle, typed, i + 3, false));
        changed_urls.push_back(rows_and_visits->back().row);
      }

      bridge()->OnURLsModified(fake_history_backend(), changed_urls);
    }

    // Check that communication with sync was successful.
    if (num_typed_urls != processor().put_multimap().size())
      return false;
    return true;
  }

  void AddObserver() {
    bridge()->history_backend_observer_.Add(fake_history_backend());
  }

  void RemoveObserver() { bridge()->history_backend_observer_.RemoveAll(); }

  // Returns specifics corresponding to the sync data for |row_and_visits|.
  // Expects success.
  static TypedUrlSpecifics ToTypedUrlSpecifics(
      const URLRowAndVisits& row_and_visits) {
    TypedUrlSpecifics specifics;
    EXPECT_TRUE(TypedURLSyncBridge::WriteToTypedUrlSpecifics(
        row_and_visits.row, row_and_visits.visits, &specifics));
    return specifics;
  }

  std::string GetStorageKey(const std::string& url) {
    return bridge()->GetStorageKeyInternal(url);
  }
  void VerifyProcessorReceivedValidEntityData() {
    for (const auto& it : processor().put_multimap()) {
      EXPECT_GT(TypedURLSyncMetadataDatabase::StorageKeyToURLID(it.first), 0);
      EXPECT_TRUE(it.second->specifics.has_typed_url());
    }
  }

  TypedUrlSpecifics GetLastUpdateForURL(const GURL& url) {
    const std::string storage_key = GetStorageKey(url.spec());
    auto eq_range = processor().put_multimap().equal_range(storage_key);
    if (eq_range.first == eq_range.second)
      return TypedUrlSpecifics();

    auto recorded_specifics_iterator = --eq_range.second;
    EXPECT_NE(processor().put_multimap().end(), recorded_specifics_iterator);
    EXPECT_TRUE(recorded_specifics_iterator->second->specifics.has_typed_url());

    return recorded_specifics_iterator->second->specifics.typed_url();
  }

  static void DiffVisits(const VisitVector& history_visits,
                         const TypedUrlSpecifics& sync_specifics,
                         std::vector<VisitInfo>* new_visits,
                         VisitVector* removed_visits) {
    TypedURLSyncBridge::DiffVisits(history_visits, sync_specifics, new_visits,
                                   removed_visits);
  }

  static TypedURLSyncBridge::MergeResult MergeUrls(
      const TypedUrlSpecifics& typed_url,
      URLRowAndVisits* row_and_visits,
      URLRow* new_url,
      std::vector<VisitInfo>* new_visits) {
    return TypedURLSyncBridge::MergeUrls(typed_url, row_and_visits->row,
                                         &row_and_visits->visits, new_url,
                                         new_visits);
  }

  static TypedUrlSpecifics MakeTypedUrlSpecifics(const GURL& url,
                                                 const std::string& title,
                                                 int64_t last_visit,
                                                 bool hidden) {
    TypedUrlSpecifics typed_url;
    typed_url.set_url(url.spec());
    typed_url.set_title(title);
    typed_url.set_hidden(hidden);
    typed_url.add_visits(last_visit);
    typed_url.add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
    return typed_url;
  }

  static const TypedURLSyncBridge::MergeResult DIFF_NONE =
      TypedURLSyncBridge::DIFF_NONE;
  static const TypedURLSyncBridge::MergeResult DIFF_UPDATE_NODE =
      TypedURLSyncBridge::DIFF_UPDATE_NODE;
  static const TypedURLSyncBridge::MergeResult DIFF_LOCAL_ROW_CHANGED =
      TypedURLSyncBridge::DIFF_LOCAL_ROW_CHANGED;
  static const TypedURLSyncBridge::MergeResult DIFF_LOCAL_VISITS_ADDED =
      TypedURLSyncBridge::DIFF_LOCAL_VISITS_ADDED;

  TypedURLSyncBridge* bridge() { return tester_.typed_url_sync_bridge(); }

  TypedURLSyncMetadataDatabase* metadata_store() {
    return bridge()->sync_metadata_database_;
  }

  RecordingModelTypeChangeProcessor& processor() { return *processor_; }

  TestHistoryBackend* fake_history_backend() {
    return tester_.fake_history_backend();
  }

 protected:
  base::MessageLoop message_loop_;
  // A non-owning pointer to the processor given to the bridge. Will be null
  // before being given to the bridge, to make ownership easier.
  RecordingModelTypeChangeProcessor* processor_ = nullptr;
  TypedURLTester tester_;
};

// Verify that GetAllData does not return expired rows.
TEST_F(TypedURLSyncBridgeTest, GetAllDataDoesNotReturnExpired) {
  auto row_and_visits = fake_history_backend()->AddRowAndVisits(
      GURL("http://expired.com/"), kTitle, 1, kExpiredVisit, false);
  EXPECT_THAT(tester_.GetAllData(), testing::IsEmpty());
}

// Add a corupted typed url locally, has typed url count 1, but no real typed
// url visit. Starting sync should not pick up this url.
TEST_F(TypedURLSyncBridgeTest, MergeUrlNoTypedUrl) {
  // Add a url to backend.
  URLRowAndVisits row_and_visits(kURL, kTitle, 0, 3, false);

  // Mark typed_count to 1 even when there is no typed url visit.
  row_and_visits.row.set_typed_count(1);
  fake_history_backend()->AddRowAndVisits(&row_and_visits);

  StartSyncing({});
  EXPECT_TRUE(processor().put_multimap().empty());

  MetadataBatch metadata_batch;
  metadata_store()->GetAllSyncMetadata(&metadata_batch);
  EXPECT_EQ(0u, metadata_batch.TakeAllMetadata().size());
}

// Add a url to the local and sync data before sync begins, with the sync data
// having more recent visits. Check that starting sync updates the backend
// with the sync visit, while the older local visit is not pushed to sync.
// The title should be updated to the sync version due to the more recent
// timestamp.
TEST_F(TypedURLSyncBridgeTest, MergeUrlOldLocal) {
  // Add a url to backend.
  URLRowAndVisits local_row_and_visits =
      fake_history_backend()->AddRowAndVisits(kURL, kTitle, 1, 3, false);

  // Create sync data for the same url with a more recent visit.
  URLRowAndVisits server_row_and_visits(kURL, kTitle2, 1, 6, false);
  server_row_and_visits.row.set_id(fake_history_backend()->GetIdByUrl(kURL));
  StartSyncing({ToTypedUrlSpecifics(server_row_and_visits)});

  // Check that the backend was updated correctly.
  VisitVector all_visits;
  base::Time server_time = base::Time::FromInternalValue(6);
  URLID url_id = fake_history_backend()->GetIdByUrl(kURL);
  ASSERT_NE(0, url_id);
  fake_history_backend()->GetVisitsForURL(url_id, &all_visits);
  ASSERT_EQ(2U, all_visits.size());
  EXPECT_EQ(server_time, all_visits.back().visit_time);
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      all_visits.back().transition,
      server_row_and_visits.visits[0].transition));
  URLRow url_row;
  EXPECT_TRUE(fake_history_backend()->GetURL(kURL, &url_row));
  EXPECT_EQ(kTitle2, base::UTF16ToUTF8(url_row.title()));

  // Check that the sync was updated correctly.
  // The local history visit should not be added to sync because it is older
  // than sync's oldest visit.
  ASSERT_EQ(1U, processor().put_multimap().size());

  TypedUrlSpecifics url_specifics = GetLastUpdateForURL(kURL);
  ASSERT_EQ(1, url_specifics.visits_size());
  EXPECT_EQ(6, url_specifics.visits(0));
  ASSERT_EQ(1, url_specifics.visit_transitions_size());
  EXPECT_EQ(static_cast<const int>(local_row_and_visits.visits[0].transition),
            url_specifics.visit_transitions(0));
}

// Add a url to the local and sync data before sync begins, with the local data
// having more recent visits. Check that starting sync updates the sync
// with the local visits, while the older sync visit is not pushed to the
// backend. Sync's title should be updated to the local version due to the more
// recent timestamp.
TEST_F(TypedURLSyncBridgeTest, MergeUrlOldSync) {
  // Add a url to backend.
  URLRowAndVisits local_row_and_visits =
      fake_history_backend()->AddRowAndVisits(kURL, kTitle2, 1, 3, false);

  // Create sync data for the same url with an older visit.
  URLRowAndVisits server_row_and_visits(kURL, kTitle, 1, 2, false);
  StartSyncing({ToTypedUrlSpecifics(server_row_and_visits)});

  // Check that the backend was not updated.
  VisitVector all_visits;
  base::Time local_visit_time = base::Time::FromInternalValue(3);
  URLID url_id = fake_history_backend()->GetIdByUrl(kURL);
  ASSERT_NE(0, url_id);
  fake_history_backend()->GetVisitsForURL(url_id, &all_visits);
  ASSERT_EQ(1U, all_visits.size());
  EXPECT_EQ(local_visit_time, all_visits[0].visit_time);

  // Check that the server was updated correctly.
  // The local history visit should not be added to sync because it is older
  // than sync's oldest visit.
  ASSERT_EQ(1U, processor().put_multimap().size());

  TypedUrlSpecifics url_specifics = GetLastUpdateForURL(kURL);
  ASSERT_EQ(1, url_specifics.visits_size());
  EXPECT_EQ(3, url_specifics.visits(0));
  EXPECT_EQ(kTitle2, url_specifics.title());
  ASSERT_EQ(1, url_specifics.visit_transitions_size());
  EXPECT_EQ(static_cast<const int>(local_row_and_visits.visits[0].transition),
            url_specifics.visit_transitions(0));
}

// Check that there is no crash during start sync, if history backend and sync
// have same url, but sync has username/password in it.
// Also check sync will not accept url with username and password.
TEST_F(TypedURLSyncBridgeTest, MergeUrlsWithUsernameAndPassword) {
  const GURL kURLWithUsernameAndPassword =
      GURL("http://username:password@pie.com/");

  // Add a url to backend.
  fake_history_backend()->AddRowAndVisits(kURL, kTitle2, 1, 3, false);

  // Create sync data for the same url but contain username and password.
  URLRowAndVisits server_row_and_visits(kURLWithUsernameAndPassword, kTitle, 1,
                                        2, false);

  // Make sure there is no crash when merge two urls.
  StartSyncing({ToTypedUrlSpecifics(server_row_and_visits)});

  // Notify typed url sync service of the update.
  bridge()->OnURLVisited(fake_history_backend(), ui::PAGE_TRANSITION_TYPED,
                         server_row_and_visits.row, RedirectList(),
                         base::Time::FromInternalValue(7));

  // Check username/password url is not synced.
  ASSERT_EQ(1U, processor().put_multimap().size());
}

// Append a RELOAD visit to a typed url that is already synced. Check that sync
// does not receive any updates.
TEST_F(TypedURLSyncBridgeTest, ReloadVisitLocalTypedUrl) {
  StartSyncing({});
  std::vector<URLRowAndVisits> rows_and_visits;
  ASSERT_TRUE(BuildAndPushLocalChanges(1, 0, {kURL}, &rows_and_visits));
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(1U, changes_multimap.size());

  // Update the URL row, adding another typed visit to the visit vector.
  fake_history_backend()->AddVisitsForUrl(
      kURL, {rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_RELOAD, 7)});

  // Notify typed url sync service of the update.
  bridge()->OnURLVisited(fake_history_backend(), ui::PAGE_TRANSITION_RELOAD,
                         rows_and_visits[0].row, RedirectList(),
                         base::Time::FromInternalValue(7));
  // No change pass to processor
  ASSERT_EQ(1U, changes_multimap.size());
}

// Appends a LINK visit to an existing typed url. Check that sync does not
// receive any changes.
TEST_F(TypedURLSyncBridgeTest, LinkVisitLocalTypedUrl) {
  StartSyncing({});
  std::vector<URLRowAndVisits> rows_and_visits;
  ASSERT_TRUE(BuildAndPushLocalChanges(1, 0, {kURL}, &rows_and_visits));
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(1U, changes_multimap.size());

  // Update the URL row, adding a non-typed visit to the visit vector.
  fake_history_backend()->AddVisitsForUrl(
      kURL, {rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_LINK, 1)});

  // Notify typed url sync service of non-typed visit, expect no change.
  bridge()->OnURLVisited(fake_history_backend(), ui::PAGE_TRANSITION_LINK,
                         rows_and_visits[0].row, RedirectList(),
                         base::Time::FromInternalValue(6));
  // No change pass to processor
  ASSERT_EQ(1U, changes_multimap.size());
}

// Appends a series of LINK visits followed by a TYPED one to an existing typed
// url. Check that sync receives an UPDATE with the newest visit data.
TEST_F(TypedURLSyncBridgeTest, TypedVisitLocalTypedUrl) {
  StartSyncing({});

  // Update the URL row, adding another typed visit to the visit vector.
  URLRowAndVisits row_and_visits(kURL, kTitle, 1, 3, false);
  row_and_visits.AddOldestVisit(ui::PAGE_TRANSITION_LINK, 1);
  row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_LINK, 6);
  row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_TYPED, 7);
  fake_history_backend()->AddRowAndVisits(&row_and_visits);

  // Notify typed url sync service of typed visit.
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(0U, changes_multimap.size());
  bridge()->OnURLVisited(fake_history_backend(), ui::PAGE_TRANSITION_TYPED,
                         row_and_visits.row, RedirectList(), base::Time::Now());

  ASSERT_EQ(1U, changes_multimap.size());
  TypedUrlSpecifics url_specifics = GetLastUpdateForURL(kURL);

  EXPECT_TRUE(URLsEqual(row_and_visits.row, url_specifics));
  EXPECT_EQ(4, url_specifics.visits_size());

  // Check that each visit has been translated/communicated correctly.
  // Note that the specifics record visits in chronological order, and the
  // visits from the db are in reverse chronological order.
  int r = url_specifics.visits_size() - 1;
  for (int i = 0; i < url_specifics.visits_size(); ++i, --r) {
    EXPECT_EQ(row_and_visits.visits[i].visit_time.ToInternalValue(),
              url_specifics.visits(r));
    EXPECT_EQ(static_cast<const int>(row_and_visits.visits[i].transition),
              url_specifics.visit_transitions(r));
  }
}

// Delete several (but not all) local typed urls. Check that sync receives the
// DELETE changes, and the non-deleted urls remain synced.
TEST_F(TypedURLSyncBridgeTest, DeleteLocalTypedUrl) {
  std::vector<URLRowAndVisits> rows_and_visits;
  std::vector<GURL> urls;
  urls.emplace_back("http://pie.com/");
  urls.emplace_back("http://cake.com/");
  urls.emplace_back("http://google.com/");
  urls.emplace_back("http://foo.com/");
  urls.emplace_back("http://bar.com/");

  StartSyncing({});
  ASSERT_TRUE(BuildAndPushLocalChanges(4, 1, urls, &rows_and_visits));
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(4U, changes_multimap.size());

  // Simulate visit expiry of typed visit, no syncing is done
  // This is to test that sync relies on the in-memory cache to know
  // which urls were typed and synced, and should be deleted.
  // TODO(mastiz) / DONOTSUBMIT: This seems broken, prior to this patch?
  // AddVisitsForUrl() is a no-op.
  rows_and_visits[0].row.set_typed_count(0);
  fake_history_backend()->AddVisitsForUrl(urls[0], VisitVector());

  // Delete some urls from backend and create deleted row vector.
  URLRows rows;
  std::set<std::string> deleted_storage_keys;
  for (size_t i = 0; i < 3u; ++i) {
    rows.push_back(rows_and_visits[i].row);
    std::string storage_key = GetStorageKey(rows.back().url().spec());
    deleted_storage_keys.insert(storage_key);
    fake_history_backend()->DeleteURL(rows.back().url());
  }

  // Notify typed url sync service.
  bridge()->OnURLsDeleted(fake_history_backend(), false, false, rows,
                          std::set<GURL>());

  const auto& delete_set = processor().delete_set();
  ASSERT_EQ(3U, delete_set.size());
  for (const std::string& storage_key : delete_set) {
    EXPECT_TRUE(deleted_storage_keys.find(storage_key) !=
                deleted_storage_keys.end());
    deleted_storage_keys.erase(storage_key);
  }
  ASSERT_TRUE(deleted_storage_keys.empty());
}

// Saturate the visits for a typed url with both TYPED and LINK navigations.
// Check that no more than kMaxTypedURLVisits are synced, and that LINK visits
// are dropped rather than TYPED ones.
TEST_F(TypedURLSyncBridgeTest, MaxVisitLocalTypedUrl) {
  StartSyncing({});
  std::vector<URLRowAndVisits> rows_and_visits;
  ASSERT_TRUE(BuildAndPushLocalChanges(0, 1, {kURL}, &rows_and_visits));
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(0U, changes_multimap.size());

  // Add |kMaxTypedUrlVisits| + 10 visits to the url. The 10 oldest
  // non-typed visits are expected to be skipped.
  VisitVector new_visits;
  int i = 1;
  for (; i <= kMaxTypedUrlVisits - 20; ++i)
    new_visits.push_back(
        rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_TYPED, i));
  for (; i <= kMaxTypedUrlVisits; ++i)
    new_visits.push_back(
        rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_LINK, i));
  for (; i <= kMaxTypedUrlVisits + 10; ++i)
    new_visits.push_back(
        rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_TYPED, i));
  fake_history_backend()->AddVisitsForUrl(kURL, new_visits);

  // Notify typed url sync service of typed visit.
  ui::PageTransition transition = ui::PAGE_TRANSITION_TYPED;
  bridge()->OnURLVisited(fake_history_backend(), transition,
                         rows_and_visits[0].row, RedirectList(),
                         base::Time::Now());

  ASSERT_EQ(1U, changes_multimap.size());
  TypedUrlSpecifics url_specifics = GetLastUpdateForURL(kURL);
  ASSERT_EQ(kMaxTypedUrlVisits, url_specifics.visits_size());

  // Check that each visit has been translated/communicated correctly.
  // Note that the specifics records visits in chronological order, and the
  // visits from the db are in reverse chronological order.
  int num_typed_visits_synced = 0;
  int num_other_visits_synced = 0;
  int r = url_specifics.visits_size() - 1;
  for (int i = 0; i < url_specifics.visits_size(); ++i, --r) {
    if (url_specifics.visit_transitions(i) ==
        static_cast<int32_t>(ui::PAGE_TRANSITION_TYPED)) {
      ++num_typed_visits_synced;
    } else {
      ++num_other_visits_synced;
    }
  }
  EXPECT_EQ(kMaxTypedUrlVisits - 10, num_typed_visits_synced);
  EXPECT_EQ(10, num_other_visits_synced);
}

// Add enough visits to trigger throttling of updates to a typed url. Check that
// sync does not receive an update until the proper throttle interval has been
// reached.
TEST_F(TypedURLSyncBridgeTest, ThrottleVisitLocalTypedUrl) {
  StartSyncing({});
  std::vector<URLRowAndVisits> rows_and_visits;
  ASSERT_TRUE(BuildAndPushLocalChanges(0, 1, {kURL}, &rows_and_visits));
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(0U, changes_multimap.size());

  // Add enough visits to the url so that typed count is above the throttle
  // limit, and not right on the interval that gets synced.
  int i = 1;
  VisitVector new_visits;
  for (; i < kVisitThrottleThreshold + kVisitThrottleMultiple / 2; ++i)
    new_visits.push_back(
        rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_TYPED, i));
  fake_history_backend()->AddVisitsForUrl(kURL, new_visits);

  // Notify typed url sync service of typed visit.
  bridge()->OnURLVisited(fake_history_backend(), ui::PAGE_TRANSITION_TYPED,
                         rows_and_visits[0].row, RedirectList(),
                         base::Time::Now());

  // Should throttle, so sync and local cache should not update.
  ASSERT_EQ(0U, changes_multimap.size());

  new_visits.clear();
  for (; i % kVisitThrottleMultiple != 1; ++i)
    new_visits.push_back(
        rows_and_visits[0].MakeVisit(ui::PAGE_TRANSITION_TYPED, i));
  --i;  // Account for the increment before the condition ends.
  fake_history_backend()->AddVisitsForUrl(kURL, new_visits);

  // Notify typed url sync service of typed visit.
  bridge()->OnURLVisited(fake_history_backend(), ui::PAGE_TRANSITION_TYPED,
                         rows_and_visits[0].row, RedirectList(),
                         base::Time::Now());

  ASSERT_EQ(1U, changes_multimap.size());
  TypedUrlSpecifics url_specifics = GetLastUpdateForURL(kURL);
  ASSERT_EQ(i, url_specifics.visits_size());
}

// Create a remote typed URL with expired visit, then send to sync bridge after
// sync has started. Check that local DB did not receive the expired URL and
// visit.
TEST_F(TypedURLSyncBridgeTest, AddExpiredUrlAndVisits) {
  StartSyncing({});

  TypedUrlSpecifics specifics = ToTypedUrlSpecifics(
      URLRowAndVisits(kURL, kTitle, 1, kExpiredVisit, false));

  ASSERT_TRUE(false);
  // tester_.ApplySyncChangesAndWait(
  //    bridge()->CreateMetadataChangeList(),
  //    {EntityChange::CreateAdd(std::string(), SpecificsToEntity(specifics))});

  ASSERT_EQ(0U, processor().put_multimap().size());
  ASSERT_EQ(0U, processor().update_multimap().size());
  ASSERT_EQ(1U, processor().untrack_set().size());

  URLID url_id = fake_history_backend()->GetIdByUrl(kURL);
  ASSERT_EQ(0, url_id);
}

// Create two set of visits for history DB and sync DB, two same set of visits
// are same. Check DiffVisits will return empty set of diff visits.
TEST_F(TypedURLSyncBridgeTest, DiffVisitsSame) {
  VisitVector old_visits;
  TypedUrlSpecifics new_url;

  const int64_t visits[] = {1024, 2065, 65534, 1237684};

  for (int64_t visit : visits) {
    old_visits.push_back(VisitRow(0, base::Time::FromInternalValue(visit), 0,
                                  ui::PAGE_TRANSITION_TYPED, 0));
    new_url.add_visits(visit);
    new_url.add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
  }

  std::vector<VisitInfo> new_visits;
  VisitVector removed_visits;

  DiffVisits(old_visits, new_url, &new_visits, &removed_visits);
  EXPECT_TRUE(new_visits.empty());
  EXPECT_TRUE(removed_visits.empty());
}

// Create two set of visits for history DB and sync DB. Check DiffVisits will
// return correct set of diff visits.
TEST_F(TypedURLSyncBridgeTest, DiffVisitsRemove) {
  VisitVector old_visits;
  TypedUrlSpecifics new_url;

  const int64_t visits_left[] = {1,    2,     1024,    1500,   2065,
                                 6000, 65534, 1237684, 2237684};
  const int64_t visits_right[] = {1024, 2065, 65534, 1237684};

  // DiffVisits will not remove the first visit, because we never delete visits
  // from the start of the array (since those visits can get truncated by the
  // size-limiting code).
  const int64_t visits_removed[] = {1500, 6000, 2237684};

  for (int64_t visit : visits_left) {
    old_visits.push_back(VisitRow(0, base::Time::FromInternalValue(visit), 0,
                                  ui::PAGE_TRANSITION_TYPED, 0));
  }

  for (int64_t visit : visits_right) {
    new_url.add_visits(visit);
    new_url.add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
  }

  std::vector<VisitInfo> new_visits;
  VisitVector removed_visits;

  DiffVisits(old_visits, new_url, &new_visits, &removed_visits);
  EXPECT_TRUE(new_visits.empty());
  ASSERT_EQ(removed_visits.size(), arraysize(visits_removed));
  for (size_t i = 0; i < arraysize(visits_removed); ++i) {
    EXPECT_EQ(removed_visits[i].visit_time.ToInternalValue(),
              visits_removed[i]);
  }
}

// Create two set of visits for history DB and sync DB. Check DiffVisits will
// return correct set of diff visits.
TEST_F(TypedURLSyncBridgeTest, DiffVisitsAdd) {
  VisitVector old_visits;
  TypedUrlSpecifics new_url;

  const int64_t visits_left[] = {1024, 2065, 65534, 1237684};
  const int64_t visits_right[] = {1,    1024,  1500,    2065,
                                  6000, 65534, 1237684, 2237684};

  const int64_t visits_added[] = {1, 1500, 6000, 2237684};

  for (int64_t visit : visits_left) {
    old_visits.push_back(VisitRow(0, base::Time::FromInternalValue(visit), 0,
                                  ui::PAGE_TRANSITION_TYPED, 0));
  }

  for (int64_t visit : visits_right) {
    new_url.add_visits(visit);
    new_url.add_visit_transitions(ui::PAGE_TRANSITION_TYPED);
  }

  std::vector<VisitInfo> new_visits;
  VisitVector removed_visits;

  DiffVisits(old_visits, new_url, &new_visits, &removed_visits);
  EXPECT_TRUE(removed_visits.empty());
  ASSERT_TRUE(new_visits.size() == arraysize(visits_added));
  for (size_t i = 0; i < arraysize(visits_added); ++i) {
    EXPECT_EQ(new_visits[i].first.ToInternalValue(), visits_added[i]);
    EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
        new_visits[i].second, ui::PAGE_TRANSITION_TYPED));
  }
}

// Create three visits, check RELOAD visit is removed by
// WriteToTypedUrlSpecifics so it won't apply to sync DB.
TEST_F(TypedURLSyncBridgeTest, WriteTypedUrlSpecifics) {
  URLRowAndVisits row_and_visits(kURL, kTitle, 0, 1, false);
  row_and_visits.visits.clear();
  row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_LINK, 1);
  row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_RELOAD, 2);
  row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_TYPED, 3);

  TypedUrlSpecifics typed_url = ToTypedUrlSpecifics(row_and_visits);

  // RELOAD visits should be removed.
  EXPECT_EQ(2, typed_url.visits_size());
  EXPECT_EQ(typed_url.visit_transitions_size(), typed_url.visits_size());
  EXPECT_EQ(1, typed_url.visits(0));
  EXPECT_EQ(3, typed_url.visits(1));
  EXPECT_EQ(static_cast<int32_t>(ui::PAGE_TRANSITION_TYPED),
            typed_url.visit_transitions(0));
  EXPECT_EQ(static_cast<int32_t>(ui::PAGE_TRANSITION_LINK),
            typed_url.visit_transitions(1));
}

// Create 101 visits, check WriteToTypedUrlSpecifics will only keep 100 visits.
TEST_F(TypedURLSyncBridgeTest, TooManyVisits) {
  int64_t timestamp = 1000;
  URLRowAndVisits row_and_visits(kURL, kTitle, 1, timestamp++, false);
  for (int i = 0; i < 100; ++i)
    row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_LINK, timestamp++);
  CHECK_EQ(101U, row_and_visits.visits.size());

  TypedUrlSpecifics typed_url = ToTypedUrlSpecifics(row_and_visits);

  // # visits should be capped at 100.
  EXPECT_EQ(100, typed_url.visits_size());
  EXPECT_EQ(typed_url.visit_transitions_size(), typed_url.visits_size());
  EXPECT_EQ(1000, typed_url.visits(0));
  // Visit with timestamp of 1001 should be omitted since we should have
  // skipped that visit to stay under the cap.
  EXPECT_EQ(1002, typed_url.visits(1));
  EXPECT_EQ(static_cast<int32_t>(ui::PAGE_TRANSITION_TYPED),
            typed_url.visit_transitions(0));
  EXPECT_EQ(static_cast<int32_t>(ui::PAGE_TRANSITION_LINK),
            typed_url.visit_transitions(1));
}

// Create 306 visits, check WriteToTypedUrlSpecifics will only keep 100 typed
// visits.
TEST_F(TypedURLSyncBridgeTest, TooManyTypedVisits) {
  int64_t timestamp = 1000;
  URLRowAndVisits row_and_visits(kURL, kTitle, 0, timestamp++, false);
  row_and_visits.visits.clear();
  for (int i = 0; i < 102; ++i) {
    row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_TYPED, timestamp++);
    row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_LINK, timestamp++);
    row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_RELOAD, timestamp++);
  }
  CHECK_EQ(306U, row_and_visits.visits.size());

  TypedUrlSpecifics typed_url = ToTypedUrlSpecifics(row_and_visits);

  // # visits should be capped at 100.
  EXPECT_EQ(100, typed_url.visits_size());
  EXPECT_EQ(typed_url.visit_transitions_size(), typed_url.visits_size());
  // First two typed visits should be skipped.
  EXPECT_EQ(1006, typed_url.visits(0));

  // Ensure there are no non-typed visits since that's all that should fit.
  for (int i = 0; i < typed_url.visits_size(); ++i) {
    EXPECT_EQ(static_cast<int32_t>(ui::PAGE_TRANSITION_TYPED),
              typed_url.visit_transitions(i));
  }
}

// Create a typed url without visit, check WriteToTypedUrlSpecifics will return
// false for it.
TEST_F(TypedURLSyncBridgeTest, NoTypedVisits) {
  URLRowAndVisits row_and_visits(kURL, kTitle, 0, 1000, false);
  TypedUrlSpecifics typed_url;
  EXPECT_FALSE(TypedURLSyncBridge::WriteToTypedUrlSpecifics(
      row_and_visits.row, VisitVector(), &typed_url));
  // URLs with no typed URL visits should not been written to specifics.
  EXPECT_EQ(0, typed_url.visits_size());
}

TEST_F(TypedURLSyncBridgeTest, MergeUrls) {
  URLRowAndVisits row_and_visits1(kURL, kTitle, 2, 3, false);
  TypedUrlSpecifics specs1(MakeTypedUrlSpecifics(kURL, kTitle, 3, false));
  URLRow new_row1((kURL));
  std::vector<VisitInfo> new_visits1;
  EXPECT_TRUE(TypedURLSyncBridgeTest::MergeUrls(specs1, &row_and_visits1,
                                                &new_row1, &new_visits1) ==
              TypedURLSyncBridgeTest::DIFF_NONE);

  URLRowAndVisits row_and_visits2(kURL, kTitle, 2, 3, false);
  TypedUrlSpecifics specs2(MakeTypedUrlSpecifics(kURL, kTitle, 3, true));
  VisitVector expected_visits2;
  URLRowAndVisits expected2(kURL, kTitle, 2, 3, true);
  URLRow new_row2((kURL));
  std::vector<VisitInfo> new_visits2;
  EXPECT_TRUE(TypedURLSyncBridgeTest::MergeUrls(specs2, &row_and_visits2,
                                                &new_row2, &new_visits2) ==
              TypedURLSyncBridgeTest::DIFF_LOCAL_ROW_CHANGED);
  EXPECT_TRUE(URLsEqual(new_row2, expected2.row));

  URLRowAndVisits row_and_visits3(kURL, kTitle, 2, 3, false);
  TypedUrlSpecifics specs3(MakeTypedUrlSpecifics(kURL, kTitle2, 3, true));
  URLRowAndVisits expected3(kURL, kTitle2, 2, 3, true);
  URLRow new_row3((kURL));
  std::vector<VisitInfo> new_visits3;
  EXPECT_EQ(TypedURLSyncBridgeTest::DIFF_LOCAL_ROW_CHANGED |
                TypedURLSyncBridgeTest::DIFF_NONE,
            TypedURLSyncBridgeTest::MergeUrls(specs3, &row_and_visits3,
                                              &new_row3, &new_visits3));
  EXPECT_TRUE(URLsEqual(new_row3, expected3.row));

  // Create one node in history DB with timestamp of 3, and one node in sync
  // DB with timestamp of 4. Result should contain one new item (4).
  URLRowAndVisits row_and_visits4(kURL, kTitle, 2, 3, false);
  TypedUrlSpecifics specs4(MakeTypedUrlSpecifics(kURL, kTitle2, 4, false));
  URLRowAndVisits expected4(kURL, kTitle2, 2, 4, false);
  URLRow new_row4((kURL));
  std::vector<VisitInfo> new_visits4;
  EXPECT_EQ(TypedURLSyncBridgeTest::DIFF_UPDATE_NODE |
                TypedURLSyncBridgeTest::DIFF_LOCAL_ROW_CHANGED |
                TypedURLSyncBridgeTest::DIFF_LOCAL_VISITS_ADDED,
            TypedURLSyncBridgeTest::MergeUrls(specs4, &row_and_visits4,
                                              &new_row4, &new_visits4));
  EXPECT_EQ(1U, new_visits4.size());
  EXPECT_EQ(specs4.visits(0), new_visits4[0].first.ToInternalValue());
  EXPECT_TRUE(URLsEqual(new_row4, expected4.row));
  EXPECT_EQ(2U, row_and_visits4.visits.size());

  URLRowAndVisits row_and_visits5(kURL, kTitle, 1, 4, false);
  TypedUrlSpecifics specs5(MakeTypedUrlSpecifics(kURL, kTitle, 3, false));
  URLRowAndVisits expected5(kURL, kTitle2, 2, 3, false);
  URLRow new_row5((kURL));
  std::vector<VisitInfo> new_visits5;

  // UPDATE_NODE should be set because row5 has a newer last_visit timestamp.
  EXPECT_EQ(TypedURLSyncBridgeTest::DIFF_UPDATE_NODE |
                TypedURLSyncBridgeTest::DIFF_NONE,
            TypedURLSyncBridgeTest::MergeUrls(specs5, &row_and_visits5,
                                              &new_row5, &new_visits5));
  EXPECT_TRUE(URLsEqual(new_row5, expected5.row));
  EXPECT_EQ(0U, new_visits5.size());
}

// Tests to ensure that we don't resurrect expired URLs (URLs that have been
// deleted from the history DB but still exist in the sync DB).
TEST_F(TypedURLSyncBridgeTest, MergeUrlsAfterExpiration) {
  // First, create a history row that has two visits, with timestamps 2 and 3.
  URLRowAndVisits row_and_visits(kURL, kTitle, 2, 2, false);
  row_and_visits.AddNewestVisit(ui::PAGE_TRANSITION_TYPED, 3);

  // Now, create a sync node with visits at timestamps 1, 2, 3, 4.
  TypedUrlSpecifics node(MakeTypedUrlSpecifics(kURL, kTitle, 1, false));
  node.add_visits(2);
  node.add_visits(3);
  node.add_visits(4);
  node.add_visit_transitions(2);
  node.add_visit_transitions(3);
  node.add_visit_transitions(4);
  URLRow new_history_url((kURL));
  std::vector<VisitInfo> new_visits;
  EXPECT_EQ(TypedURLSyncBridgeTest::DIFF_NONE |
                TypedURLSyncBridgeTest::DIFF_LOCAL_VISITS_ADDED,
            TypedURLSyncBridgeTest::MergeUrls(node, &row_and_visits,
                                              &new_history_url, &new_visits));
  EXPECT_TRUE(URLsEqual(row_and_visits.row, new_history_url));
  EXPECT_EQ(1U, new_visits.size());
  EXPECT_EQ(4U, new_visits[0].first.ToInternalValue());
  // We should not sync the visit with timestamp #1 since it is earlier than
  // any other visit for this URL in the history DB. But we should sync visit
  // #4.
  ASSERT_EQ(3U, row_and_visits.visits.size());
  EXPECT_EQ(2U, row_and_visits.visits[0].visit_time.ToInternalValue());
  EXPECT_EQ(3U, row_and_visits.visits[1].visit_time.ToInternalValue());
  EXPECT_EQ(4U, row_and_visits.visits[2].visit_time.ToInternalValue());
}

// Create a local typed URL with one expired TYPED visit,
// MergeSyncData should not pass it to sync. And then add a non
// expired visit, OnURLsModified should only send the non expired visit to sync.
TEST_F(TypedURLSyncBridgeTest, LocalExpiredTypedUrlDoNotSync) {
  // Add an expired typed URL to local.
  fake_history_backend()->AddRowAndVisits(kURL, kTitle, 1, kExpiredVisit,
                                          false);

  StartSyncing({});

  // Check change processor did not receive expired typed URL.
  const auto& changes_multimap = processor().put_multimap();
  ASSERT_EQ(0U, changes_multimap.size());

  // Add a non expired typed URL to local.
  URLRowAndVisits row_and_visits =
      fake_history_backend()->AddRowAndVisits(kURL, kTitle, 2, 1, false);

  // Notify typed url sync service of the update.
  bridge()->OnURLsModified(fake_history_backend(),
                           /*changed_urls=*/{row_and_visits.row});

  // Check change processor did not receive expired typed URL.
  ASSERT_EQ(1U, changes_multimap.size());

  // Get typed url specifics. Verify only a non-expired visit received.
  TypedUrlSpecifics url_specifics = GetLastUpdateForURL(kURL);

  EXPECT_TRUE(URLsEqual(row_and_visits.row, url_specifics));
  ASSERT_EQ(1, url_specifics.visits_size());
  ASSERT_EQ(static_cast<const int>(row_and_visits.visits.size() - 1),
            url_specifics.visits_size());
  EXPECT_EQ(row_and_visits.visits[1].visit_time.ToInternalValue(),
            url_specifics.visits(0));
  EXPECT_EQ(static_cast<const int>(row_and_visits.visits[1].transition),
            url_specifics.visit_transitions(0));
}

// Tests that database error gets reported to processor as model type error.
TEST_F(TypedURLSyncBridgeTest, DatabaseError) {
  processor().ExpectError();
  bridge()->OnDatabaseError();
}

}  // namespace history

namespace syncer {

INSTANTIATE_TEST_CASE_P(
    TypedURL,
    SyncBridgeTest,
    ::testing::Values(
        ModelTypeSyncBridgeTester::GetFactory<history::TypedURLTester>()));

}  // namespace syncer
