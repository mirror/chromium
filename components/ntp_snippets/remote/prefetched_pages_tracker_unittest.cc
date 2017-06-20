// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/prefetched_pages_tracker.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "components/ntp_snippets/offline_pages/offline_pages_test_utils.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using ntp_snippets::test::FakeOfflinePageModel;
using offline_pages::OfflinePageItem;

namespace ntp_snippets {

namespace {

OfflinePageItem CreateOfflinePageItem(const GURL& url,
                                      const std::string& name_space) {
  static int id = 0;
  ++id;
  return OfflinePageItem(
      url, id, offline_pages::ClientId(name_space, base::IntToString(id)),
      base::FilePath::FromUTF8Unsafe(
          base::StringPrintf("some/folder/%d.mhtml", id)),
      0, base::Time::Now());
}

}  // namespace

class PrefetchedPagesTrackerTest : public ::testing::Test {
 public:
  PrefetchedPagesTrackerTest()
      : fake_offline_page_model_(base::MakeUnique<FakeOfflinePageModel>()){};

  FakeOfflinePageModel* fake_offline_page_model() {
    return fake_offline_page_model_.get();
  }

 private:
  std::unique_ptr<FakeOfflinePageModel> fake_offline_page_model_;
  DISALLOW_COPY_AND_ASSIGN(PrefetchedPagesTrackerTest);
};

TEST_F(PrefetchedPagesTrackerTest,
       ShouldRetrievePrefetchedEarlierSuggestionsOnStartup) {
  (*fake_offline_page_model()->mutable_items()) = {
      CreateOfflinePageItem(GURL("http://prefetched.com"),
                            offline_pages::kSuggestedArticlesNamespace)};
  PrefetchedPagesTracker tracker(fake_offline_page_model());
  ASSERT_FALSE(tracker.OfflinePageExists(GURL("http://not_added_url.com")));
  EXPECT_TRUE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
}

TEST_F(PrefetchedPagesTrackerTest, ShouldAddNewPrefetchedPagesWhenNotified) {
  fake_offline_page_model()->mutable_items()->clear();
  PrefetchedPagesTracker tracker(fake_offline_page_model());
  ASSERT_FALSE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
  tracker.OfflinePageAdded(
      fake_offline_page_model(),
      CreateOfflinePageItem(GURL("http://prefetched.com"),
                            offline_pages::kSuggestedArticlesNamespace));
  EXPECT_TRUE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
}

TEST_F(PrefetchedPagesTrackerTest,
       ShouldIgnoreOtherTypesOfOfflinePagesWhenNotified) {
  fake_offline_page_model()->mutable_items()->clear();
  PrefetchedPagesTracker tracker(fake_offline_page_model());
  ASSERT_FALSE(
      tracker.OfflinePageExists(GURL("http://manually_downloaded.com")));
  tracker.OfflinePageAdded(
      fake_offline_page_model(),
      CreateOfflinePageItem(GURL("http://manually_downloaded.com"),
                            offline_pages::kNTPSuggestionsNamespace));
  EXPECT_FALSE(
      tracker.OfflinePageExists(GURL("http://manually_downloaded.com")));
}

TEST_F(PrefetchedPagesTrackerTest,
       ShouldIgnoreOtherTypesOfOfflinePagesOnStartup) {
  (*fake_offline_page_model()->mutable_items()) = {
      CreateOfflinePageItem(GURL("http://manually_downloaded.com"),
                            offline_pages::kNTPSuggestionsNamespace)};
  PrefetchedPagesTracker tracker(fake_offline_page_model());
  ASSERT_FALSE(
      tracker.OfflinePageExists(GURL("http://manually_downloaded.com")));
  EXPECT_FALSE(
      tracker.OfflinePageExists(GURL("http://manually_downloaded.com")));
}

TEST_F(PrefetchedPagesTrackerTest, ShouldDeletePrefetchedURLWhenNotified) {
  const OfflinePageItem item =
      CreateOfflinePageItem(GURL("http://prefetched.com"),
                            offline_pages::kSuggestedArticlesNamespace);
  (*fake_offline_page_model()->mutable_items()) = {item};
  PrefetchedPagesTracker tracker(fake_offline_page_model());
  ASSERT_TRUE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
  tracker.OfflinePageDeleted(item.offline_id, item.client_id);
  EXPECT_FALSE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
}

TEST_F(PrefetchedPagesTrackerTest,
       ShouldIgnoreDeletionOfOtherTypeOfflinePagesWhenNotified) {
  const OfflinePageItem prefetched_item =
      CreateOfflinePageItem(GURL("http://prefetched.com"),
                            offline_pages::kSuggestedArticlesNamespace);
  // The URL is intentionally the same.
  const OfflinePageItem manually_downloaded_item = CreateOfflinePageItem(
      GURL("http://prefetched.com"), offline_pages::kNTPSuggestionsNamespace);
  (*fake_offline_page_model()->mutable_items()) = {prefetched_item,
                                                   manually_downloaded_item};
  PrefetchedPagesTracker tracker(fake_offline_page_model());
  ASSERT_TRUE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
  tracker.OfflinePageDeleted(manually_downloaded_item.offline_id,
                             manually_downloaded_item.client_id);
  EXPECT_TRUE(tracker.OfflinePageExists(GURL("http://prefetched.com")));
}

}  // namespace ntp_snippets
