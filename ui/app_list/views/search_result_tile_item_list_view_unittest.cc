// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/search_result_tile_item_list_view.h"

#include <map>
#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "ui/app_list/app_list_features.h"
#include "ui/app_list/app_list_model.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/test/test_search_result.h"
#include "ui/app_list/views/search_result_list_view_delegate.h"
#include "ui/app_list/views/search_result_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/test/views_test_base.h"

namespace app_list {
namespace test {

namespace {
const int kMaxNumSearchResultTiles = 6;
const int kInstalledApps = 4;
const int kPlayStoreApps = 2;
}  // namespace

class SearchResultTileItemListViewTest : public views::ViewsTestBase {
 public:
  SearchResultTileItemListViewTest() {}
  ~SearchResultTileItemListViewTest() override {}

  // Overridden from testing::Test:
  void SetUp() override { views::ViewsTestBase::SetUp(); }

 protected:
  void CreateSearchResultTileItemListView() {
    textfield_.reset(new views::Textfield());
    view_.reset(
        new SearchResultTileItemListView(textfield_.get(), &view_delegate_));
    view_->SetResults(view_delegate_.GetModel()->results());
  }

  void EnablePlayStoreAppSearch() {
    scoped_feature_list_.InitAndEnableFeature(
        app_list::features::kEnablePlayStoreAppSearch);
  }

  SearchResultTileItemListView* view() { return view_.get(); }

  AppListModel::SearchResults* GetResults() {
    return view_delegate_.GetModel()->results();
  }

  void SetUpSearchResults() {
    AppListModel::SearchResults* results = GetResults();

    // Populate results for installed applications.
    for (int i = 0; i < kInstalledApps; ++i) {
      std::unique_ptr<TestSearchResult> result =
          base::MakeUnique<TestSearchResult>();
      result->set_display_type(SearchResult::DISPLAY_TILE);
      result->set_result_type(SearchResult::RESULT_INSTALLED_APP);
      result->set_title(
          base::UTF8ToUTF16(base::StringPrintf("InstalledApp %d", i)));
      results->Add(std::move(result));
    }

    // Populate results for Play Store search applications.
    if (app_list::features::IsPlayStoreAppSearchEnabled()) {
      for (int i = 0; i < kPlayStoreApps; ++i) {
        std::unique_ptr<TestSearchResult> result =
            base::MakeUnique<TestSearchResult>();
        result->set_display_type(SearchResult::DISPLAY_TILE);
        result->set_result_type(SearchResult::RESULT_PLAYSTORE_APP);
        result->set_title(
            base::UTF8ToUTF16(base::StringPrintf("PlayStoreApp %d", i)));
        results->Add(std::move(result));
      }
    }

    // Adding results will schedule Update().
    RunPendingMessages();
    view_->OnContainerSelected(false, false);
  }

  int GetOpenResultCount(int ranking) {
    int result = view_delegate_.open_search_result_counts()[ranking];
    return result;
  }

  void ResetOpenResultCount() {
    view_delegate_.open_search_result_counts().clear();
  }

  int GetResultCount() { return view_->num_results(); }

  int GetSelectedIndex() { return view_->selected_index(); }

  void ResetSelectedIndex() { view_->SetSelectedIndex(0); }

  bool KeyPress(ui::KeyboardCode key_code) {
    ui::KeyEvent event(ui::ET_KEY_PRESSED, key_code, ui::EF_NONE);
    return view_->OnKeyPressed(event);
  }

 private:
  AppListTestViewDelegate view_delegate_;
  std::unique_ptr<SearchResultTileItemListView> view_;
  std::unique_ptr<views::Textfield> textfield_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(SearchResultTileItemListViewTest);
};

TEST_F(SearchResultTileItemListViewTest, Basic) {
  EnablePlayStoreAppSearch();
  CreateSearchResultTileItemListView();
  SetUpSearchResults();

  const int results = GetResultCount();
  EXPECT_EQ(kInstalledApps + kPlayStoreApps, results);
  // For each results, we added a separator for result type grouping.
  EXPECT_EQ(kMaxNumSearchResultTiles * 2, view()->child_count());

  // Tests item indexing.
  for (int i = 1; i < results; ++i) {
    EXPECT_TRUE(KeyPress(ui::VKEY_TAB));
    EXPECT_EQ(i, GetSelectedIndex());
  }

  // Tests app opening.
  ResetSelectedIndex();
  ResetOpenResultCount();
  for (int i = 1; i < results; ++i) {
    EXPECT_TRUE(KeyPress(ui::VKEY_TAB));
    EXPECT_EQ(i, GetSelectedIndex());
    for (int j = 0; j < i; j++) {
      EXPECT_TRUE(KeyPress(ui::VKEY_RETURN));
    }
  }
  for (int i = 0; i < results; ++i) {
    EXPECT_EQ(i, GetOpenResultCount(i));
  }
}

}  // namespace test
}  // namespace app_list
