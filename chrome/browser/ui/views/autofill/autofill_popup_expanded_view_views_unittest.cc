// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_expanded_view_views.h"

#include "build/build_config.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "components/autofill/core/browser/suggestion.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"

class MockAutofillPopupController : public autofill::AutofillPopupController {
 public:
  MockAutofillPopupController() {
    gfx::FontList::SetDefaultFontDescription("Arial, Times New Roman, 15px");
    layout_model_.reset(new autofill::AutofillPopupLayoutModel(
        this, false /* is_credit_card_field */));
  }

  // AutofillPopupViewDelegate
  MOCK_METHOD0(Hide, void());
  MOCK_METHOD0(ViewDestroyed, void());
  MOCK_METHOD1(SetSelectionAtPoint, void(const gfx::Point& point));
  MOCK_METHOD0(AcceptSelectedLine, bool());
  MOCK_METHOD0(SelectionCleared, void());
  MOCK_CONST_METHOD0(popup_bounds, gfx::Rect());
  MOCK_METHOD0(container_view, gfx::NativeView());
  MOCK_CONST_METHOD0(element_bounds, const gfx::RectF&());
  MOCK_CONST_METHOD0(IsRTL, bool());
  const std::vector<autofill::Suggestion> GetSuggestions() override {
    std::vector<autofill::Suggestion> suggestions(
        GetLineCount(), autofill::Suggestion("", "", "", 0));
    return suggestions;
  }
#if !defined(OS_ANDROID)
  MOCK_METHOD1(GetElidedValueWidthForRow, int(int row));
  MOCK_METHOD1(GetElidedLabelWidthForRow, int(int row));
#endif

  // AutofillPopupController
  MOCK_METHOD0(OnSuggestionsChanged, void());
  MOCK_METHOD1(AcceptSuggestion, void(int index));
  MOCK_CONST_METHOD0(GetLineCount, int());
  const autofill::Suggestion& GetSuggestionAt(int row) const override {
    return suggestion_;
  }
  MOCK_CONST_METHOD1(GetElidedValueAt, const base::string16&(int row));
  MOCK_CONST_METHOD1(GetElidedLabelAt, const base::string16&(int row));
  MOCK_METHOD3(GetRemovalConfirmationText,
               bool(int index, base::string16* title, base::string16* body));
  MOCK_METHOD1(RemoveSuggestion, bool(int index));
  MOCK_CONST_METHOD1(GetBackgroundColorIDForRow,
                     ui::NativeTheme::ColorId(int index));
  MOCK_CONST_METHOD0(selected_line, base::Optional<int>());
  const autofill::AutofillPopupLayoutModel& layout_model() const override {
    return *layout_model_;
  }

 private:
  std::unique_ptr<autofill::AutofillPopupLayoutModel> layout_model_;
  autofill::Suggestion suggestion_;
};

class AutofillPopupExpandedViewViewsTest : public views::ViewsTestBase {
 public:
  AutofillPopupExpandedViewViewsTest() = default;
  ~AutofillPopupExpandedViewViewsTest() override = default;

  void SetUp() override {
    views::ViewsTestBase::SetUp();
    widget_.reset(new views::Widget);
    views::Widget::InitParams init_params =
        CreateParams(views::Widget::InitParams::TYPE_POPUP);
    init_params.ownership =
        views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    widget_->Init(init_params);
    view_ = new autofill::AutofillPopupExpandedViewViews(
        &autofill_popup_controller_, widget_.get());
  }

  void TearDown() override {
    widget_.reset();
    views::ViewsTestBase::TearDown();
  }

  autofill::AutofillPopupExpandedViewViews* view() { return view_; }

 private:
  autofill::AutofillPopupExpandedViewViews* view_;
  MockAutofillPopupController autofill_popup_controller_;
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupExpandedViewViewsTest);
};

TEST_F(AutofillPopupExpandedViewViewsTest, ShowHideTest) {
  view()->Show();
  view()->Hide();
}
