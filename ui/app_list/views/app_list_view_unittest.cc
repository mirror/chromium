// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/app_list_view.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/app_list_constants.h"
#include "ui/app_list/app_list_features.h"
#include "ui/app_list/pagination_model.h"
#include "ui/app_list/search_box_model.h"
#include "ui/app_list/test/app_list_test_model.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/test/test_search_result.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/apps_container_view.h"
#include "ui/app_list/views/apps_grid_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/app_list/views/expand_arrow_view.h"
#include "ui/app_list/views/search_box_view.h"
#include "ui/app_list/views/search_result_page_view.h"
#include "ui/app_list/views/search_result_tile_item_view.h"
#include "ui/app_list/views/start_page_view.h"
#include "ui/app_list/views/suggestions_container_view.h"
#include "ui/app_list/views/tile_item_view.h"
#include "ui/events/event_utils.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/test/views_test_base.h"

namespace app_list {
namespace test {

namespace {

// Choose a set that is 3 regular app list pages and 2 landscape app list pages.
constexpr int kInitialItems = 34;

template <class T>
size_t GetVisibleViews(const std::vector<T*>& tiles) {
  size_t count = 0;
  for (const auto& tile : tiles) {
    if (tile->visible())
      count++;
  }
  return count;
}

// A standard set of checks on a view, e.g., ensuring it is drawn and visible.
void CheckView(views::View* subview) {
  ASSERT_TRUE(subview);
  EXPECT_TRUE(subview->parent());
  EXPECT_TRUE(subview->visible());
  EXPECT_TRUE(subview->IsDrawn());
  EXPECT_FALSE(subview->bounds().IsEmpty());
}

void SimulateClick(views::View* view) {
  gfx::Point center = view->GetLocalBounds().CenterPoint();
  view->OnMousePressed(ui::MouseEvent(
      ui::ET_MOUSE_PRESSED, center, center, ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
  view->OnMouseReleased(ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, center, center, ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON));
}

class TestStartPageSearchResult : public TestSearchResult {
 public:
  TestStartPageSearchResult() { set_display_type(DISPLAY_RECOMMENDATION); }
  ~TestStartPageSearchResult() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestStartPageSearchResult);
};

class AppListViewTest : public views::ViewsTestBase,
                        public testing::WithParamInterface<bool> {
 public:
  AppListViewTest() = default;
  ~AppListViewTest() override = default;

  // testing::Test overrides:
  void SetUp() override {
    views::ViewsTestBase::SetUp();

    // If the current test is parameterized.
    if (testing::UnitTest::GetInstance()->current_test_info()->value_param()) {
      test_with_fullscreen_ = GetParam();
      if (test_with_fullscreen_)
        EnableFullscreenAppList();
    }

    gfx::NativeView parent = GetContext();
    delegate_.reset(new AppListTestViewDelegate);
    view_ = new AppListView(delegate_.get());

    view_->Initialize(parent, 0, false, false);
    // Initialize around a point that ensures the window is wholly shown. It
    // bails out early with |test_with_fullscreen_|.
    // TODO(warx): remove MaybeSetAnchorPoint setup here when bubble launcher is
    // removed from code base.
    gfx::Size size = view_->bounds().size();
    view_->MaybeSetAnchorPoint(gfx::Point(size.width() / 2, size.height() / 2));

    main_view_ = view_->app_list_main_view();
    contents_view_ = main_view_->contents_view();
  }

  // testing::Test overrides:
  void TearDown() override {
    view_->GetWidget()->Close();
    views::ViewsTestBase::TearDown();
  }

 protected:
  void EnableFullscreenAppList() {
    scoped_feature_list_.InitAndEnableFeature(
        features::kEnableFullscreenAppList);
  }

  // Switches the launcher to |state| and lays out to ensure all launcher pages
  // are in the correct position. Checks that the state is where it should be
  // and returns false on failure.
  bool SetAppListState(AppListModel::State state) {
    contents_view_->SetActiveState(state);
    contents_view_->Layout();
    return IsStateShown(state);
  }

  // Returns true if all of the pages are in their correct position for |state|.
  bool IsStateShown(AppListModel::State state) {
    bool success = true;
    for (int i = 0; i < contents_view_->NumLauncherPages(); ++i) {
      success = success &&
                (contents_view_->GetPageView(i)->GetPageBoundsForState(state) ==
                 contents_view_->GetPageView(i)->bounds());
    }
    return success && state == delegate_->GetTestModel()->state();
  }

  // Checks the search box widget is at |expected| in the contents view's
  // coordinate space.
  bool CheckSearchBoxWidget(const gfx::Rect& expected) {
    // Adjust for the search box view's shadow.
    gfx::Rect expected_with_shadow =
        view_->app_list_main_view()
            ->search_box_view()
            ->GetViewBoundsForSearchBoxContentsBounds(expected);
    gfx::Point point = expected_with_shadow.origin();
    views::View::ConvertPointToScreen(contents_view_, &point);

    return gfx::Rect(point, expected_with_shadow.size()) ==
           view_->search_box_widget()->GetWindowBoundsInScreen();
  }

  // Gets the PaginationModel owned by |view_|.
  PaginationModel* GetPaginationModel() const {
    return view_->GetAppsPaginationModel();
  }

  AppListView* view_ = nullptr;  // Owned by native widget.
  AppListMainView* main_view_ = nullptr;   // Owned by |view_|.
  ContentsView* contents_view_ = nullptr;  // Owned by |view_|.
  std::unique_ptr<AppListTestViewDelegate> delegate_;
  bool test_with_fullscreen_ = false;
  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppListViewTest);
};

}  // namespace

// Instantiate the Boolean which is used to toggle the Fullscreen app list in
// the parameterized tests.
INSTANTIATE_TEST_CASE_P(, AppListViewTest, testing::Bool());

// Tests displaying the app list and performs a standard set of checks on its
// top level views.
TEST_P(AppListViewTest, DisplayTest) {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  delegate_->GetTestModel()->PopulateApps(kInitialItems);

  view_->GetWidget()->Show();

  if (!test_with_fullscreen_) {
    // For bubble launcher, explicitly enforce the exact dimensions of the app
    // list. Feel free to change these if you need to (they are just here to
    // prevent against accidental changes to the window size).
    EXPECT_EQ("768x570", view_->bounds().size().ToString());
  } else {
    // For fullscreen launcher, |view_| bounds equal to the root window's
    // fullscreen size.
    EXPECT_EQ("800x648", view_->bounds().size().ToString());
  }

  EXPECT_EQ(2, GetPaginationModel()->total_pages());
  EXPECT_EQ(0, GetPaginationModel()->selected_page());

  // Checks on the main view.
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view_));
  EXPECT_NO_FATAL_FAILURE(CheckView(contents_view_));

  AppListModel::State expected = AppListModel::STATE_START;
  EXPECT_TRUE(contents_view_->IsStateActive(expected));
  EXPECT_EQ(expected, delegate_->GetTestModel()->state());
}

// Tests that the start page view operates correctly.
TEST_P(AppListViewTest, StartPageTest) {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  AppListTestModel* model = delegate_->GetTestModel();
  model->PopulateApps(3);

  view_->GetWidget()->Show();

  StartPageView* start_page_view = contents_view_->start_page_view();
  // Checks on the main view.
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view_));
  EXPECT_NO_FATAL_FAILURE(CheckView(contents_view_));
  EXPECT_NO_FATAL_FAILURE(CheckView(start_page_view));

  // Show the start page view.
  EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));

  if (!test_with_fullscreen_) {
    // The "All apps" button should have its "parent background color" set to
    // the tiles container's background color.
    TileItemView* all_apps_button = start_page_view->all_apps_button();
    EXPECT_TRUE(all_apps_button->visible());
    EXPECT_EQ(kLabelBackgroundColor,
              all_apps_button->parent_background_color());

    // Simulate clicking the "All apps" button. Check that we navigate to the
    // apps grid view.
    SimulateClick(all_apps_button);
    contents_view_->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));
  } else {
    ExpandArrowView* expand_arrow = start_page_view->expand_arrow_for_test();
    EXPECT_TRUE(expand_arrow->visible());
    // Simulate clicking the expand arrow view. Check that we navigate to the
    // apps grid view.
    SimulateClick(expand_arrow);
    contents_view_->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));
  }

  // Check suggested tiles hide and show on deletion and addition.
  SuggestionsContainerView* start_page_suggestions_container =
      start_page_view->suggestions_container_for_test();
  EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
  model->results()->Add(base::MakeUnique<TestStartPageSearchResult>());
  start_page_view->UpdateForTesting();
  EXPECT_EQ(1u,
            GetVisibleViews(start_page_suggestions_container->tile_views()));
  model->results()->DeleteAll();
  start_page_view->UpdateForTesting();
  EXPECT_EQ(0u,
            GetVisibleViews(start_page_suggestions_container->tile_views()));

  if (!test_with_fullscreen_) {
    // Tiles should not update when the start page is not active but should be
    // correct once the start page is shown.
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_APPS));
    model->results()->Add(base::MakeUnique<TestStartPageSearchResult>());
    start_page_view->UpdateForTesting();
    EXPECT_EQ(0u,
              GetVisibleViews(start_page_suggestions_container->tile_views()));
    EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
    EXPECT_EQ(1u,
              GetVisibleViews(start_page_suggestions_container->tile_views()));
  }
  // TODO(warx): for |test_with_fullscreen_|, we need to update to use single
  // suggestions_container for both start page and all apps page.
}

// Tests switching rapidly between multiple pages of the launcher.
TEST_P(AppListViewTest, PageSwitchingAnimationTest) {
  // Checks on the main view.
  EXPECT_NO_FATAL_FAILURE(CheckView(main_view_));
  EXPECT_NO_FATAL_FAILURE(CheckView(contents_view_));

  contents_view_->SetActiveState(AppListModel::STATE_START);
  contents_view_->Layout();

  IsStateShown(AppListModel::STATE_START);

  // Change pages. View should not have moved without Layout().
  contents_view_->SetActiveState(AppListModel::STATE_SEARCH_RESULTS);
  IsStateShown(AppListModel::STATE_START);

  // Change to a third page. This queues up the second animation behind the
  // first.
  contents_view_->SetActiveState(AppListModel::STATE_APPS);
  IsStateShown(AppListModel::STATE_START);

  // Call Layout(). Should jump to the third page.
  contents_view_->Layout();
  IsStateShown(AppListModel::STATE_APPS);
}

// Tests that the correct views are displayed for showing search results.
TEST_P(AppListViewTest, SearchResultsTest) {
  if (test_with_fullscreen_)
    return;
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());
  AppListTestModel* model = delegate_->GetTestModel();
  model->PopulateApps(3);

  view_->GetWidget()->Show();
  EXPECT_TRUE(SetAppListState(AppListModel::STATE_APPS));

  // Show the search results.
  contents_view_->ShowSearchResults(true);
  contents_view_->Layout();
  EXPECT_TRUE(
      contents_view_->IsStateActive(AppListModel::STATE_SEARCH_RESULTS));

  EXPECT_TRUE(IsStateShown(AppListModel::STATE_SEARCH_RESULTS));

  // Hide the search results.
  contents_view_->ShowSearchResults(false);
  contents_view_->Layout();

  // Check that we return to the page that we were on before the search.
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));

  // Check that typing into the search box triggers the search page.
  EXPECT_TRUE(SetAppListState(AppListModel::STATE_START));
  view_->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));

  base::string16 search_text = base::UTF8ToUTF16("test");
  main_view_->search_box_view()->search_box()->SetText(base::string16());
  main_view_->search_box_view()->search_box()->InsertText(search_text);
  // Check that the current search is using |search_text|.
  EXPECT_EQ(search_text, delegate_->GetTestModel()->search_box()->text());
  EXPECT_EQ(search_text, main_view_->search_box_view()->search_box()->text());
  contents_view_->Layout();
  EXPECT_TRUE(
      contents_view_->IsStateActive(AppListModel::STATE_SEARCH_RESULTS));
  EXPECT_TRUE(
      CheckSearchBoxWidget(contents_view_->GetDefaultSearchBoxBounds()));

  // Check that typing into the search box triggers the search page.
  EXPECT_TRUE(SetAppListState(AppListModel::STATE_APPS));
  contents_view_->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_APPS));
  EXPECT_TRUE(
      CheckSearchBoxWidget(contents_view_->GetDefaultSearchBoxBounds()));

  base::string16 new_search_text = base::UTF8ToUTF16("apple");
  main_view_->search_box_view()->search_box()->SetText(base::string16());
  main_view_->search_box_view()->search_box()->InsertText(new_search_text);
  // Check that the current search is using |new_search_text|.
  EXPECT_EQ(new_search_text, delegate_->GetTestModel()->search_box()->text());
  EXPECT_EQ(new_search_text,
            main_view_->search_box_view()->search_box()->text());
  contents_view_->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_SEARCH_RESULTS));
  EXPECT_TRUE(
      CheckSearchBoxWidget(contents_view_->GetDefaultSearchBoxBounds()));
}

// Tests that the back button navigates through the app list correctly.
TEST_P(AppListViewTest, BackTest) {
  EXPECT_FALSE(view_->GetWidget()->IsVisible());
  EXPECT_EQ(-1, GetPaginationModel()->total_pages());

  view_->GetWidget()->Show();

  AppListMainView* main_view = view_->app_list_main_view();
  ContentsView* contents_view = main_view->contents_view();
  SearchBoxView* search_box_view = main_view->search_box_view();

  // Show the apps grid.
  SetAppListState(AppListModel::STATE_APPS);
  if (!test_with_fullscreen_) {
    EXPECT_NO_FATAL_FAILURE(CheckView(search_box_view->back_button()));

    // The back button should return to the start page.
    EXPECT_TRUE(contents_view->Back());
    contents_view->Layout();
    EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));
    EXPECT_FALSE(search_box_view->back_button()->visible());

    // Show the apps grid again.
    SetAppListState(AppListModel::STATE_APPS);
    EXPECT_NO_FATAL_FAILURE(CheckView(search_box_view->back_button()));
  }

  // Pressing ESC should return to the start page.
  view_->AcceleratorPressed(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  contents_view->Layout();
  EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));
  if (!test_with_fullscreen_)
    EXPECT_FALSE(search_box_view->back_button()->visible());

  // Pressing ESC from the start page should close the app list.
  EXPECT_EQ(0, delegate_->dismiss_count());
  view_->AcceleratorPressed(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  EXPECT_EQ(1, delegate_->dismiss_count());

  // Show the search results.
  // base::string16 new_search_text = base::UTF8ToUTF16("apple");
  // search_box_view->search_box()->SetText(base::string16());
  // search_box_view->search_box()->InsertText(new_search_text);
  // contents_view->Layout();
  // EXPECT_TRUE(IsStateShown(AppListModel::STATE_SEARCH_RESULTS));

  //// The back button should return to the start page.
  // if (!test_with_fullscreen_) {
  // EXPECT_NO_FATAL_FAILURE(CheckView(search_box_view->back_button()));
  // EXPECT_TRUE(contents_view->Back());
  //} else {
  // view_->AcceleratorPressed(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
  //}
  // contents_view->Layout();
  // EXPECT_TRUE(IsStateShown(AppListModel::STATE_START));
  // EXPECT_FALSE(search_box_view->back_button()->visible());
}

// Tests that the correct views are displayed for showing search results.
TEST_P(AppListViewTest, AppListOverlayTest) {
  view_->GetWidget()->Show();

  AppListMainView* main_view = view_->app_list_main_view();
  SearchBoxView* search_box_view = main_view->search_box_view();

  // The search box should not be enabled when the app list overlay is shown.
  view_->SetAppListOverlayVisible(true);
  EXPECT_FALSE(search_box_view->enabled());

  // The search box should be refocused when the app list overlay is hidden.
  view_->SetAppListOverlayVisible(false);
  EXPECT_TRUE(search_box_view->enabled());
  EXPECT_EQ(search_box_view->search_box(),
            view_->GetWidget()->GetFocusManager()->GetFocusedView());
}

}  // namespace test
}  // namespace app_list
