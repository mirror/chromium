// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "signin_manager_android.h"

#include <memory>
#include <set>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browsing_data/browsing_data_cache_storage_helper.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate_factory.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

class SigninManagerAndroidTest : public ::testing::Test {
 public:
  SigninManagerAndroidTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~SigninManagerAndroidTest() override{};

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
  }

  TestingProfile* CreateTestingProfile() {
    return profile_manager_.CreateTestingProfile("Testing Profile");
  }

  static std::unique_ptr<KeyedService> GetEmptyBrowsingDataRemoverDelegate(
      content::BrowserContext* context) {
    return nullptr;
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager profile_manager_;

  DISALLOW_COPY_AND_ASSIGN(SigninManagerAndroidTest);
};

TEST_F(SigninManagerAndroidTest, DeleteGoogleServiceWorkerCaches) {
  // Owned by |profile_manager_|.
  TestingProfile* profile = CreateTestingProfile();

  struct TestCase {
    std::string worker_url;
    bool should_be_deleted;
  } kTestCases[] = {
      // A Google domain.
      {"https://google.com/foo/bar", true},

      // A Google domain with long TLD.
      {"https://plus.google.co.uk/?query_params", true},

      // Youtube.
      {"https://youtube.com", false},

      // A random domain.
      {"https://a.b.c.example.com", false},

      // Another Google domain.
      {"https://www.google.de/worker.html", true},

      // Ports don't matter, only TLDs.
      {"https://google.com:8444/worker.html", true},
  };

  // Add service workers.
  scoped_refptr<CannedBrowsingDataCacheStorageHelper> helper(
      new CannedBrowsingDataCacheStorageHelper(
          content::BrowserContext::GetDefaultStoragePartition(profile)
              ->GetCacheStorageContext()));

  for (const TestCase& test_case : kTestCases)
    helper->AddCacheStorage(GURL(test_case.worker_url));

  ASSERT_EQ(arraysize(kTestCases), helper->GetCacheStorageCount());

  // Delete service workers and wait for completion.
  base::RunLoop run_loop;
  SigninManagerAndroid::WipeData(profile,
                                 false /* only Google service worker caches */,
                                 run_loop.QuitClosure());
  run_loop.Run();

  // Test whether the correct service worker caches were deleted.
  std::set<std::string> remaining_cache_storages;
  for (const auto& info : helper->GetCacheStorageUsageInfo())
    remaining_cache_storages.insert(info.origin.spec());

  for (const TestCase& test_case : kTestCases) {
    EXPECT_EQ(test_case.should_be_deleted,
              base::ContainsKey(remaining_cache_storages, test_case.worker_url))
        << test_case.worker_url << " should "
        << (test_case.should_be_deleted ? "" : "NOT ")
        << "be deleted, but it was"
        << (test_case.should_be_deleted ? "NOT" : "") << ".";
  }
}

TEST_F(SigninManagerAndroidTest, Bookmarks) {
  // Owned by |profile_manager_|.
  TestingProfile* profile = CreateTestingProfile();

  // ChromeBrowsingDataRemoverDelegate deletes a lot of data storage backends
  // that will not work correctly in a unittest. Remove it. Note that
  // bookmarks deletion is currently not performed in the delegate, so this
  // test remains valid.
  ChromeBrowsingDataRemoverDelegateFactory::GetInstance()
      ->SetTestingFactoryAndUse(
          profile,
          SigninManagerAndroidTest::GetEmptyBrowsingDataRemoverDelegate);

  // Add some bookmarks.
  profile->CreateBookmarkModel(true);
  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile);
  bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);

  bookmark_model->AddURL(bookmark_model->bookmark_bar_node(), 0,
                         base::ASCIIToUTF16("Example 1"),
                         GURL("https://example.org/1"));
  bookmark_model->AddURL(bookmark_model->bookmark_bar_node(), 1,
                         base::ASCIIToUTF16("Example 2"),
                         GURL("https://example.com/2"));
  EXPECT_EQ(2, bookmark_model->bookmark_bar_node()->child_count());

  // Delete service workers and wait for completion. This should NOT delete
  // bookmarks.
  std::unique_ptr<base::RunLoop> run_loop(new base::RunLoop());
  SigninManagerAndroid::WipeData(profile,
                                 false /* only Google service worker caches */,
                                 run_loop->QuitClosure());
  run_loop->Run();
  EXPECT_EQ(2, bookmark_model->bookmark_bar_node()->child_count());

  // Delete all data and wait for completion. This should delete bookmarks as
  // well.
  run_loop.reset(new base::RunLoop());
  SigninManagerAndroid::WipeData(profile, true /* all data */,
                                 run_loop->QuitClosure());
  run_loop->Run();
  EXPECT_EQ(0, bookmark_model->bookmark_bar_node()->child_count());
}
