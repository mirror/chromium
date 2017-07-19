// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/browsing_history_service.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/history/browsing_history_service_handler.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/web_history_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/fake_web_history_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/sync/driver/fake_sync_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::Time;
using base::TimeDelta;
using history::HistoryService;
using history::WebHistoryService;

using HistoryEntry = BrowsingHistoryService::HistoryEntry;
#define COMBINED HistoryEntry::COMBINED_ENTRY
#define LOCAL HistoryEntry::LOCAL_ENTRY
#define REMOTE HistoryEntry::REMOTE_ENTRY

namespace {

const char kUrl1[] = "http://www.one.com";
const char kUrl2[] = "http://www.two.com";
const char kUrl3[] = "http://www.three.com";
const char kUrl4[] = "http://www.four.com";
const char kUrl5[] = "http://www.five.com";
const char kUrl6[] = "http://www.six.com";
const char kUrl7[] = "http://www.seven.com";

struct TestResult {
  std::string url;
  int64_t hour_offset;  // Visit time in hours past the baseline time.
  HistoryEntry::EntryType type;
};

// For each item in |results|, create a new Value representing the visit, and
// insert it into |list_value|.
void AddQueryResults(const Time baseline_time_,
                     TestResult* test_results,
                     int test_results_size,
                     std::vector<HistoryEntry>* results) {
  for (int i = 0; i < test_results_size; ++i) {
    HistoryEntry entry;
    entry.time =
        baseline_time_ + TimeDelta::FromHours(test_results[i].hour_offset);
    entry.url = GURL(test_results[i].url);
    entry.all_timestamps.insert(entry.time.ToInternalValue());
    entry.entry_type = test_results[i].type;
    results->push_back(entry);
  }
}

// Returns true if |result| matches the test data given by |correct_result|,
// otherwise returns false.
void VerifyEquals(const Time baseline_time_,
                  const HistoryEntry& result,
                  const TestResult& correct_result) {
  Time correct_time =
      baseline_time_ + TimeDelta::FromHours(correct_result.hour_offset);
  EXPECT_EQ(correct_time, result.time);
  EXPECT_EQ(GURL(correct_result.url), result.url);
}

// TODO(skym): This was copied/pasted from browsing_history_handler_unittest.cc,
// find some way to share this.
static std::unique_ptr<KeyedService> BuildFakeWebHistoryService(
    content::BrowserContext* context) {
  Profile* profile = static_cast<TestingProfile*>(context);

  std::unique_ptr<history::FakeWebHistoryService> service =
      base::MakeUnique<history::FakeWebHistoryService>(
          ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
          SigninManagerFactory::GetForProfile(profile),
          profile->GetRequestContext());
  service->SetupFakeResponse(true /* success */, net::HTTP_OK);
  return std::move(service);
}

class FakeHandler : public BrowsingHistoryServiceHandler {
 public:
  // BrowsingHistoryServiceHandler implementation.
  void OnQueryComplete(std::vector<HistoryEntry>* results,
                       const BrowsingHistoryService::QueryResultsInfo&
                           query_results_info) override {
    last_query_results_ = *results;
    last_query_info_ = query_results_info;
  }
  void OnRemoveVisitsComplete() override {}
  void OnRemoveVisitsFailed() override {}
  void HistoryDeleted() override {}
  void HasOtherFormsOfBrowsingHistory(bool has_other_forms,
                                      bool has_synced_results) override {}

  std::vector<HistoryEntry> last_query_results() const {
    return last_query_results_;
  }
  BrowsingHistoryService::QueryResultsInfo last_query_info() const {
    return last_query_info_;
  }

 private:
  std::vector<HistoryEntry> last_query_results_;
  BrowsingHistoryService::QueryResultsInfo last_query_info_;
};

class TestSyncService : public syncer::FakeSyncService {
 public:
  bool IsSyncActive() const override { return true; }

  syncer::ModelTypeSet GetActiveDataTypes() const override {
    return {syncer::HISTORY_DELETE_DIRECTIVES};
  }
};

static std::unique_ptr<KeyedService> BuildFakeSyncService(
    content::BrowserContext* context) {
  return base::MakeUnique<TestSyncService>();
}

}  // namespace

class BrowsingHistoryServiceTest : public ::testing::Test {
 public:
  BrowsingHistoryServiceTest() {
    ProfileSyncServiceFactory::GetInstance()->SetTestingFactory(
        &profile_, &BuildFakeSyncService);
    EXPECT_TRUE(profile_.CreateHistoryService(true, false));
    history_service_ = HistoryServiceFactory::GetForProfile(
        &profile_, ServiceAccessType::EXPLICIT_ACCESS);
    web_history_service_ = static_cast<history::FakeWebHistoryService*>(
        WebHistoryServiceFactory::GetInstance()->SetTestingFactoryAndUse(
            &profile_, &BuildFakeWebHistoryService));

    browsing_history_service_ =
        base::MakeUnique<BrowsingHistoryService>(&profile_, &handler_);

    // LocalMidnight() may take us before UnixEpoch(), which runs amok with some
    // logic that's trying to sanitize inputs.
    baseline_time_ = Time::UnixEpoch().LocalMidnight() + TimeDelta::FromDays(1);
  }
  ~BrowsingHistoryServiceTest() override {}

 protected:
  std::vector<HistoryEntry> MergeAndVerify(
      std::vector<TestResult> test_data,
      std::vector<int> expected_result_indexes,
      std::vector<int> expected_pending_indexes,
      bool reached_beggining_local = true,
      bool reached_beggining_remote = true) {
    std::vector<HistoryEntry> actual_results;
    std::vector<HistoryEntry> actual_pending;

    AddQueryResults(baseline_time_, &test_data[0], test_data.size(),
                    &actual_results);
    BrowsingHistoryService::MergeDuplicateResults(
        &actual_results, &actual_pending, reached_beggining_local,
        reached_beggining_remote);

    EXPECT_EQ(expected_result_indexes.size(), actual_results.size());
    for (size_t i = 0; i < expected_result_indexes.size(); ++i) {
      VerifyEquals(baseline_time_, actual_results[i],
                   test_data[expected_result_indexes[i]]);
    }
    EXPECT_EQ(expected_pending_indexes.size(), actual_pending.size());
    for (size_t i = 0; i < expected_pending_indexes.size(); ++i) {
      VerifyEquals(baseline_time_, actual_pending[i],
                   test_data[expected_pending_indexes[i]]);
    }
    return actual_results;
  }

  void SetupHistory(std::vector<TestResult> test_data) {
    for (const TestResult& data : test_data) {
      Time time = baseline_time_ + TimeDelta::FromHours(data.hour_offset);
      if (data.type == LOCAL) {
        history_service_->AddPage(GURL(data.url), time,
                                  history::VisitSource::SOURCE_BROWSED);
      } else if (data.type == REMOTE) {
        web_history_service_->AddSyncedVisit(data.url, time);
      } else {
        NOTREACHED();
      }
    }
  }

  void QueryHistory(size_t max_count, bool incremental, Time end_time) {
    history::QueryOptions options;
    options.max_count = max_count;
    options.end_time = end_time;
    options.duplicate_policy = history::QueryOptions::REMOVE_DUPLICATES_PER_DAY;
    browsing_history_service_->QueryHistory(base::UTF8ToUTF16(""), options,
                                            incremental);
    profile_.BlockUntilHistoryProcessesPendingRequests();
  }

  void VerifyHistory(std::vector<TestResult> expected,
                     std::vector<HistoryEntry> actual) {
    EXPECT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < std::min(expected.size(), actual.size()); ++i) {
      VerifyEquals(baseline_time_, actual[i], expected[i]);
    }
  }

  const FakeHandler& handler() { return handler_; }
  HistoryService* history_service() { return history_service_; }
  history::FakeWebHistoryService* web_history_service() {
    return web_history_service_;
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  FakeHandler handler_;
  std::unique_ptr<BrowsingHistoryService> browsing_history_service_;
  HistoryService* history_service_;
  history::FakeWebHistoryService* web_history_service_;
  Time baseline_time_;
};

// Basic test that duplicates on the same day are removed.
TEST_F(BrowsingHistoryServiceTest, MergeSameDayDuplicates) {
  MergeAndVerify({{kUrl1, 0, LOCAL},
                  {kUrl2, 1, LOCAL},
                  {kUrl1, 2, LOCAL},
                  {kUrl1, 3, LOCAL}},
                 {3, 1}, {});
}

// Test that a duplicate URL on the next day is not removed.
TEST_F(BrowsingHistoryServiceTest, MergeDifferentDayDuplicates) {
  MergeAndVerify({{kUrl1, 0, LOCAL}, {kUrl1, 23, LOCAL}, {kUrl1, 24, LOCAL}},
                 {2, 1}, {});
}

// Test multiple duplicates across multiple days.
TEST_F(BrowsingHistoryServiceTest, MergeManyDuplicates) {
  MergeAndVerify(
      {// First day.
       {kUrl2, 0, LOCAL},
       {kUrl1, 1, LOCAL},
       {kUrl2, 2, LOCAL},
       {kUrl1, 3, LOCAL},
       // Second day.
       {kUrl2, 24, LOCAL},
       {kUrl1, 25, LOCAL},
       {kUrl2, 26, LOCAL},
       {kUrl1, 27, LOCAL}},
      {7, 6, 3, 2}, {});
}

// Test that timestamps for duplicates are properly saved.
TEST_F(BrowsingHistoryServiceTest, MergeDuplicateTimestamps) {
  auto results = MergeAndVerify({{kUrl1, 0, LOCAL},
                                 {kUrl2, 1, LOCAL},
                                 {kUrl1, 2, LOCAL},
                                 {kUrl1, 3, LOCAL}},
                                {3, 1}, {});
  EXPECT_EQ(3u, results[0].all_timestamps.size());
  EXPECT_EQ(1u, results[1].all_timestamps.size());
}

// Tests that local entries are dropped only when we haven't reached the end of
// remote data yet.
TEST_F(BrowsingHistoryServiceTest, MergeDropOnLocal) {
  std::vector<TestResult> test_data = {{kUrl1, 0, LOCAL},
                                       {kUrl2, 1, REMOTE},
                                       {kUrl3, 2, LOCAL},
                                       {kUrl4, 3, REMOTE}};
  MergeAndVerify(test_data, {3, 2, 1}, {0}, false, false);
  MergeAndVerify(test_data, {3, 2, 1}, {0}, true, false);
  MergeAndVerify(test_data, {3, 2, 1, 0}, {}, false, true);
  MergeAndVerify(test_data, {3, 2, 1, 0}, {}, true, true);
}

// Tests that remote entries are dropped only when we haven't reached the end of
// local data yet.
TEST_F(BrowsingHistoryServiceTest, MergeDropOnRemoteOldest) {
  std::vector<TestResult> test_data = {{kUrl1, 0, REMOTE},
                                       {kUrl2, 1, REMOTE},
                                       {kUrl3, 2, LOCAL},
                                       {kUrl4, 3, REMOTE}};
  MergeAndVerify(test_data, {3, 2}, {1, 0}, false, false);
  MergeAndVerify(test_data, {3, 2, 1, 0}, {}, true, false);
  MergeAndVerify(test_data, {3, 2}, {1, 0}, false, true);
  MergeAndVerify(test_data, {3, 2, 1, 0}, {}, true, true);
}

// Time entry 0 is going to be deduplicated, but it is important that we do not
// send time entry 1 to pending.
TEST_F(BrowsingHistoryServiceTest, MergeDropAwareOfDuplicates) {
  std::vector<TestResult> test_data = {{kUrl1, 0, LOCAL},
                                       {kUrl2, 1, REMOTE},
                                       {kUrl1, 2, LOCAL},
                                       {kUrl3, 3, REMOTE}};
  MergeAndVerify(test_data, {3, 2, 1}, {}, false, false);
}

TEST_F(BrowsingHistoryServiceTest, QueryHistoryMerge) {
  SetupHistory({{kUrl1, 1, REMOTE},
                {kUrl2, 2, REMOTE},
                {kUrl3, 3, LOCAL},
                {kUrl1, 4, LOCAL}});

  QueryHistory(2, false, Time());
  VerifyHistory({{kUrl1, 4, COMBINED}, {kUrl3, 3, LOCAL}, {kUrl2, 2, REMOTE}},
                handler().last_query_results());
}

TEST_F(BrowsingHistoryServiceTest, QueryHistoryPending) {
  SetupHistory({{kUrl1, 1, REMOTE},
                {kUrl2, 2, REMOTE},
                {kUrl3, 3, LOCAL},
                {kUrl4, 4, LOCAL}});

  QueryHistory(1, false, Time());
  VerifyHistory({{kUrl4, 4, LOCAL}}, handler().last_query_results());

  QueryHistory(1, true, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl3, 3, LOCAL}, {kUrl2, 2, REMOTE}},
                handler().last_query_results());

  QueryHistory(1, true, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl1, 1, REMOTE}}, handler().last_query_results());
}

// A full request worth of local results will sit in pending, resulting in us
// being able to delete local history before our next query and we should still
// see the local entry.
TEST_F(BrowsingHistoryServiceTest, QueryHistoryFullLocalPending) {
  SetupHistory({{kUrl1, 1, LOCAL}, {kUrl2, 2, REMOTE}, {kUrl3, 3, REMOTE}});

  QueryHistory(1, false, Time());
  VerifyHistory({{kUrl3, 3, REMOTE}}, handler().last_query_results());

  history_service()->DeleteURL(GURL(kUrl1));

  QueryHistory(1, true, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl2, 2, REMOTE}, {kUrl1, 1, LOCAL}},
                handler().last_query_results());
}

// Part of a request worth of local results will sit in pending, resulting in us
// seeing extra local results on our next request.
TEST_F(BrowsingHistoryServiceTest, QueryHistoryPartialLocalPending) {
  SetupHistory({{kUrl1, 1, LOCAL},
                {kUrl2, 2, LOCAL},
                {kUrl3, 3, REMOTE},
                {kUrl4, 4, LOCAL},
                {kUrl5, 5, REMOTE},
                {kUrl6, 6, REMOTE},
                {kUrl7, 7, LOCAL}});

  QueryHistory(2, false, Time());
  VerifyHistory({{kUrl7, 7, LOCAL}, {kUrl6, 6, REMOTE}, {kUrl5, 5, REMOTE}},
                handler().last_query_results());

  QueryHistory(2, true, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl4, 4, LOCAL},
                 {kUrl3, 3, REMOTE},
                 {kUrl2, 2, LOCAL},
                 {kUrl1, 1, LOCAL}},
                handler().last_query_results());
}

// A full request worth of remote results will sit in pending, resulting in us
// being able to delete remote history before our next query and we should still
// see the remote entry.
TEST_F(BrowsingHistoryServiceTest, QueryHistoryFullRemotePending) {
  SetupHistory({{kUrl1, 1, REMOTE}, {kUrl2, 2, LOCAL}, {kUrl3, 3, LOCAL}});

  QueryHistory(1, false, Time());
  VerifyHistory({{kUrl3, 3, LOCAL}}, handler().last_query_results());

  web_history_service()->ClearSyncedVisits();

  QueryHistory(1, true, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl2, 2, LOCAL}, {kUrl1, 1, REMOTE}},
                handler().last_query_results());
}

// Part of a request worth of remote results will sit in pending, resulting in
// us seeing extra remote results on our next request.
TEST_F(BrowsingHistoryServiceTest, QueryHistoryPartialRemotePending) {
  SetupHistory({{kUrl1, 1, REMOTE},
                {kUrl2, 2, REMOTE},
                {kUrl3, 3, LOCAL},
                {kUrl4, 4, REMOTE},
                {kUrl5, 5, LOCAL},
                {kUrl6, 6, LOCAL},
                {kUrl7, 7, REMOTE}});

  QueryHistory(2, false, Time());
  VerifyHistory({{kUrl7, 7, REMOTE}, {kUrl6, 6, LOCAL}, {kUrl5, 5, LOCAL}},
                handler().last_query_results());

  QueryHistory(2, true, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl4, 4, REMOTE},
                 {kUrl3, 3, LOCAL},
                 {kUrl2, 2, REMOTE},
                 {kUrl1, 1, REMOTE}},
                handler().last_query_results());
}

// While we hold pending remote results in pending, by not asking for an
// "incremental" query we do not re-use the pending data, and instead re-query.
// We verify this by putting new data into remote history.
TEST_F(BrowsingHistoryServiceTest, QueryHistoryNonIncrementalBreaksPending) {
  SetupHistory({{kUrl1, 1, REMOTE}, {kUrl2, 2, LOCAL}, {kUrl3, 3, LOCAL}});

  QueryHistory(1, false, Time());
  VerifyHistory({{kUrl3, 3, LOCAL}}, handler().last_query_results());

  web_history_service()->ClearSyncedVisits();
  SetupHistory({{kUrl4, 1, REMOTE}});

  QueryHistory(1, false, handler().last_query_results().rbegin()->time);
  VerifyHistory({{kUrl2, 2, LOCAL}, {kUrl4, 1, REMOTE}},
                handler().last_query_results());
}

#undef COMBINED
#undef LOCAL
#undef REMOTE
