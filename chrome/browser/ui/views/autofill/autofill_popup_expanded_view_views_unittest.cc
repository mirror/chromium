// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_expanded_view_views.h"

#include "build/build_config.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/views/test/views_test_base.h"

struct TypeClicks {
  autofill::PopupItemId id;
  bool click;
};

const struct TypeClicks kClickTestCase[] = {
    {autofill::POPUP_ITEM_ID_AUTOCOMPLETE_ENTRY, true},
    {autofill::POPUP_ITEM_ID_INSECURE_CONTEXT_PAYMENT_DISABLED_MESSAGE, true},
    {autofill::POPUP_ITEM_ID_PASSWORD_ENTRY, true},
    {autofill::POPUP_ITEM_ID_SEPARATOR, false},
    {autofill::POPUP_ITEM_ID_CLEAR_FORM, true},
    {autofill::POPUP_ITEM_ID_AUTOFILL_OPTIONS, true},
    {autofill::POPUP_ITEM_ID_DATALIST_ENTRY, true},
    {autofill::POPUP_ITEM_ID_SCAN_CREDIT_CARD, true},
    {autofill::POPUP_ITEM_ID_TITLE, true},
    {autofill::POPUP_ITEM_ID_CREDIT_CARD_SIGNIN_PROMO, true},
    {autofill::POPUP_ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE, true},
    {autofill::POPUP_ITEM_ID_USERNAME_ENTRY, true},
    {autofill::POPUP_ITEM_ID_CREATE_HINT, true},
    {autofill::POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY, true},
    {autofill::POPUP_ITEM_ID_GENERATE_PASSWORD_ENTRY, true},
};

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

  int GetLineCount() const override { return line_count_; }

  const autofill::Suggestion& GetSuggestionAt(int row) const override {
    return suggestion_;
  }

  const base::string16& GetElidedValueAt(int i) const override {
    return suggestion_.value;
  }
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

  void set_line_count(int line_count) { line_count_ = line_count; }
  void set_suggestion_id(int id) { suggestion_.frontend_id = id; }

 private:
  int line_count_;
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
  }

  void TearDown() override {
    widget_.reset();
    views::ViewsTestBase::TearDown();
  }

  void CreateAndShowView(int lines, int id = 0) {
    autofill_popup_controller_.set_line_count(1);
    autofill_popup_controller_.set_suggestion_id(id);
    view_ = new autofill::AutofillPopupExpandedViewViews(
        &autofill_popup_controller_, widget_.get());
    view_->Show();
  }

  autofill::AutofillPopupExpandedViewViews* view() { return view_; }

 protected:
  autofill::AutofillPopupExpandedViewViews* view_;
  MockAutofillPopupController autofill_popup_controller_;
  std::unique_ptr<views::Widget> widget_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AutofillPopupExpandedViewViewsTest);
};

class AutofillPopupExpandedViewViewsForEveryTypeTest
    : public AutofillPopupExpandedViewViewsTest,
      public ::testing::WithParamInterface<TypeClicks> {};

TEST_F(AutofillPopupExpandedViewViewsTest, ShowHideTest) {
  CreateAndShowView(1);
  EXPECT_CALL(autofill_popup_controller_, AcceptSuggestion(testing::_))
      .Times(0);
  view()->Hide();
}

TEST_P(AutofillPopupExpandedViewViewsForEveryTypeTest, ShowClickTest) {
  const TypeClicks& click = GetParam();
  CreateAndShowView(1, click.id);
  EXPECT_CALL(autofill_popup_controller_, AcceptSuggestion(0))
      .Times(static_cast<int>(click.click));
  gfx::Point center = view()->GetLocalBounds().CenterPoint();
  ui::MouseEvent released_event = ui::MouseEvent(
      ui::ET_MOUSE_RELEASED, center, center, ui::EventTimeForNow(),
      ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);

  view()->child_at(0)->OnMouseReleased(released_event);
}

INSTANTIATE_TEST_CASE_P(
    /* no prefix */,
    AutofillPopupExpandedViewViewsForEveryTypeTest,
    ::testing::ValuesIn(kClickTestCase));
