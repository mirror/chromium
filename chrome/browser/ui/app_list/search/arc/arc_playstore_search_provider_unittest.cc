// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/arc_playstore_search_provider.h"

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/arc/arc_app_test.h"
#include "chrome/browser/ui/app_list/test/test_app_list_controller_delegate.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/search_result.h"

namespace app_list {

class ArcPlayStoreSearchProviderTest : public AppListTestBase {
 public:
  ArcPlayStoreSearchProviderTest() {}
  ~ArcPlayStoreSearchProviderTest() override {}

  // AppListTestBase:
  void SetUp() override {
    AppListTestBase::SetUp();
    arc_test_.SetUp(profile());
    controller_ = base::MakeUnique<test::TestAppListControllerDelegate>();
  }

  // AppListTestBase:
  void TearDown() override {
    controller_.reset();
    arc_test_.TearDown();
    AppListTestBase::TearDown();
  }

  void CreateSearch(int max_results) {
    app_search_ = base::MakeUnique<ArcPlayStoreSearchProvider>(
        max_results, profile_.get(), controller_.get());
  }

  std::string RunQuery(const std::string& query) {
    app_search_->Start(false, base::UTF8ToUTF16(query));

    std::vector<SearchResult*> sorted_results;
    std::string result_str;
    for (const auto& result : app_search_->results()) {
      sorted_results.push_back(result.get());

      if (!result_str.empty())
        result_str += ',';

      result_str += base::UTF16ToUTF8(result->title());
    }
    return result_str;
  }

  const SearchProvider::Results& results() { return app_search_->results(); }

 private:
  std::unique_ptr<ArcPlayStoreSearchProvider> app_search_;
  std::unique_ptr<test::TestAppListControllerDelegate> controller_;
  ArcAppTest arc_test_;

  DISALLOW_COPY_AND_ASSIGN(ArcPlayStoreSearchProviderTest);
};

TEST_F(ArcPlayStoreSearchProviderTest, Basic) {
  const int MAX_RESULTS = 3;
  const std::string QUERY = "Play App";

  CreateSearch(MAX_RESULTS);
  EXPECT_TRUE(results().empty());

  // Checks that results should be properly stored when we search for
  // "Play App".
  std::string result_string = "";
  for (int i = 0; i < MAX_RESULTS; ++i) {
    result_string += QUERY + " " + std::to_string(i);
    if (i < MAX_RESULTS - 1)
      result_string += ",";
  }
  EXPECT_EQ(result_string, RunQuery(QUERY));

  // Checks that information is correctly set in each result.
  for (int i = 0; i < MAX_RESULTS; ++i) {
    const bool is_instant_app = i % 2 == 0;
    EXPECT_EQ(base::UTF16ToUTF8(results()[i]->title()),
              QUERY + " " + std::to_string(i));
    EXPECT_EQ(results()[i]->display_type(), SearchResult::DISPLAY_TILE);
    EXPECT_EQ(base::UTF16ToUTF8(results()[i]->formatted_price()),
              "$" + std::to_string(i) + ".22");
    EXPECT_EQ(results()[i]->rating(), i);
    EXPECT_EQ(results()[i]->result_type(),
              is_instant_app ? SearchResult::RESULT_INSTANT_APP
                             : SearchResult::RESULT_PLAYSTORE_APP);
  }
}

}  // namespace app_list
