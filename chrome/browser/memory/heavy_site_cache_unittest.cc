// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/heavy_site_cache.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace memory {

class HeavySiteCacheTest : public testing::Test {
 public:
  HeavySiteCacheTest() {}

  std::unique_ptr<HeavySiteCache> CreateCache(size_t max_cache_size) {
    auto cache =
        std::make_unique<HeavySiteCache>(&testing_profile_, max_cache_size);
    auto test_clock = base::MakeUnique<base::SimpleTestClock>();
    test_clock->Advance(base::TimeDelta::FromMilliseconds(100));
    test_clock_ = test_clock.get();
    cache->set_clock_for_testing(std::move(test_clock));
    return cache;
  }

  TestingProfile* profile() { return &testing_profile_; }
  base::SimpleTestClock* clock() { return test_clock_; }

  HeavySiteCache* cache_ = nullptr;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile testing_profile_;
  base::SimpleTestClock* test_clock_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(HeavySiteCacheTest);
};

// It isn't very cheap to initialize the history service. Tests that need it can
// use this harness.
class HeavySiteCacheHistoryTest : public HeavySiteCacheTest {
 public:
  void SetUp() override {
    ASSERT_TRUE(profile()->CreateHistoryService(true /* delete_file */,
                                                false /* no_db */));
    HeavySiteCacheTest::SetUp();
  }
};

TEST_F(HeavySiteCacheTest, ZeroMaxSize) {
  auto cache = CreateCache(0u);
  const GURL url("https://example.test/");
  cache->AddSite(url);
  EXPECT_FALSE(cache->IsSiteHeavy(url));
}

TEST_F(HeavySiteCacheTest, EvictOldEntries) {
  auto cache = CreateCache(3u);
  const GURL url1("https://example1.test/");
  const GURL url2("https://example2.test/");
  const GURL url3("https://example3.test/");
  const GURL url4("https://example4.test/");

  cache->AddSite(url1);
  EXPECT_TRUE(cache->IsSiteHeavy(url1));
  clock()->Advance(base::TimeDelta::FromMilliseconds(10));

  cache->AddSite(url2);
  EXPECT_TRUE(cache->IsSiteHeavy(url2));
  clock()->Advance(base::TimeDelta::FromMilliseconds(10));

  cache->AddSite(url3);
  EXPECT_TRUE(cache->IsSiteHeavy(url3));
  clock()->Advance(base::TimeDelta::FromMilliseconds(10));

  cache->AddSite(url4);
  clock()->Advance(base::TimeDelta::FromMilliseconds(10));

  // url1 should be evicted.
  EXPECT_FALSE(cache->IsSiteHeavy(url1));
  EXPECT_TRUE(cache->IsSiteHeavy(url2));
  EXPECT_TRUE(cache->IsSiteHeavy(url3));
  EXPECT_TRUE(cache->IsSiteHeavy(url4));

  // Add back url1 and url2 should be evicted.
  cache->AddSite(url1);
  EXPECT_TRUE(cache->IsSiteHeavy(url1));
  EXPECT_FALSE(cache->IsSiteHeavy(url2));
  EXPECT_TRUE(cache->IsSiteHeavy(url3));
  EXPECT_TRUE(cache->IsSiteHeavy(url4));
}

TEST_F(HeavySiteCacheTest, PersistsAcrossCaches) {
  const GURL url1("https://example1.test/");
  const GURL url2("https://example2.test/");

  {
    auto cache1 = CreateCache(3u);
    cache1->AddSite(url1);
  }
  {
    auto cache2 = CreateCache(3u);
    cache2->AddSite(url2);
  }
  auto cache3 = CreateCache(3u);
  EXPECT_TRUE(cache3->IsSiteHeavy(url1));
  EXPECT_TRUE(cache3->IsSiteHeavy(url2));
}

TEST_F(HeavySiteCacheTest, CorruptCache) {
  const GURL previous_url("https://previous.test/");
  {
    auto previous_cache = CreateCache(1u);
    previous_cache->AddSite(previous_url);
  }

  const GURL url1("https://example1.test/");
  const GURL url2("https://example2.test/");

  auto dict1 = std::make_unique<base::DictionaryValue>();
  dict1->SetDouble("random", 1.0);
  HostContentSettingsMapFactory::GetForProfile(profile())
      ->SetWebsiteSettingDefaultScope(url1, GURL(),
                                      CONTENT_SETTINGS_TYPE_HEAVY_MEMORY,
                                      std::string(), std::move(dict1));
  auto dict2 = std::make_unique<base::DictionaryValue>();
  dict2->SetBoolean("nogoodpath", false);
  HostContentSettingsMapFactory::GetForProfile(profile())
      ->SetWebsiteSettingDefaultScope(url2, GURL(),
                                      CONTENT_SETTINGS_TYPE_HEAVY_MEMORY,
                                      std::string(), std::move(dict2));

  auto cache = CreateCache(3u);
  EXPECT_FALSE(cache->IsSiteHeavy(url1));
  EXPECT_FALSE(cache->IsSiteHeavy(url2));
  EXPECT_FALSE(cache->IsSiteHeavy(previous_url));
}

TEST_F(HeavySiteCacheHistoryTest, HistoryUrlDeleted_ClearsWebsiteSetting) {
  auto cache = CreateCache(10u);

  // Simulate a history already populated with a URL.
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile(), ServiceAccessType::EXPLICIT_ACCESS);
  ASSERT_TRUE(history_service);
  history_service->AddPage(GURL("https://already-browsed.com/"),
                           base::Time::Now(), history::SOURCE_BROWSED);

  // Ensure the website setting is set.
  GURL url("https://example.test");
  cache->AddSite(url);
  EXPECT_TRUE(cache->IsSiteHeavy(url));

  // Simulate adding the page to the history.
  history_service->AddPage(url, base::Time::Now(), history::SOURCE_BROWSED);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);

  // Deleting the URL from history should clear the setting for this URL. Note
  // that since there is another URL in the history this won't clear all items.
  history_service->DeleteURL(url);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);

  EXPECT_FALSE(cache->IsSiteHeavy(url));
}

TEST_F(HeavySiteCacheHistoryTest, AllHistoryUrlDeleted_ClearsWebsiteSetting) {
  auto cache = CreateCache(10u);

  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile(), ServiceAccessType::EXPLICIT_ACCESS);
  ASSERT_TRUE(history_service);

  GURL url1("https://example.test");
  GURL url2("https://example2.test");
  cache->AddSite(url1);
  cache->AddSite(url2);
  EXPECT_TRUE(cache->IsSiteHeavy(url1));
  EXPECT_TRUE(cache->IsSiteHeavy(url2));

  // Simulate adding the pages to the history.
  history_service->AddPage(url1, base::Time::Now(), history::SOURCE_BROWSED);
  history_service->AddPage(url2, base::Time::Now(), history::SOURCE_BROWSED);
  history::BlockUntilHistoryProcessesPendingRequests(history_service);

  // Deleting all the URLs should clear everything.
  base::RunLoop run_loop;
  base::CancelableTaskTracker task_tracker;
  history_service->ExpireHistoryBetween(std::set<GURL>(), base::Time(),
                                        base::Time(), run_loop.QuitClosure(),
                                        &task_tracker);
  run_loop.Run();

  EXPECT_FALSE(cache->IsSiteHeavy(url1));
  EXPECT_FALSE(cache->IsSiteHeavy(url2));
}

}  // namespace memory
