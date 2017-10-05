// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_expanded_view_views.h"

#include "base/strings/string16.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/text_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace autofill {

namespace {

class AutofillPopupChildView : public views::View {
 public:
  explicit AutofillPopupChildView(AutofillPopupController* controller,
                                  const Suggestion& suggestion,
                                  int line_number)
      : controller_(controller),
        suggestion_(suggestion),
        line_number_(line_number) {
    SetFocusBehavior(FocusBehavior::ALWAYS);
  }

  ~AutofillPopupChildView() override {}

  void ClearSelection() {}
  void AcceptSelection() { controller_->AcceptSuggestion(line_number_); }

  // views::Views implementation.
  // TODO(melandory): These actions are duplicates of what is implemented in
  // AutofillPopupBaseView. Once migration is finished, code in
  // AutofillPopupBaseView should be cleaned up.
  void OnMouseCaptureLost() override { ClearSelection(); }

  bool OnMouseDragged(const ui::MouseEvent& event) override { return true; }

  void OnMouseExited(const ui::MouseEvent& event) override {}

  void OnMouseMoved(const ui::MouseEvent& event) override {}

  bool OnMousePressed(const ui::MouseEvent& event) override { return true; }

  void OnMouseReleased(const ui::MouseEvent& event) override {
    if (event.IsOnlyLeftMouseButton() && HitTestPoint(event.location()))
      AcceptSelection();
  }

  void OnGestureEvent(ui::GestureEvent* event) override {}

  bool AcceleratorPressed(const ui::Accelerator& accelerator) override {
    return true;
  }

 private:
  // views::Views implementation
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->role = ui::AX_ROLE_MENU_ITEM;
    node_data->SetName(suggestion_.value);
  }

  AutofillPopupController* controller_;
  const Suggestion& suggestion_;
  int line_number_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupChildView);
};

class AutofillPopupTextOnlyView : public AutofillPopupChildView {
 public:
  explicit AutofillPopupTextOnlyView(AutofillPopupController* controller,
                                     int line_number,
                                     const Suggestion& suggestion,
                                     const base::string16& text,
                                     SkColor text_color,
                                     SkColor background,
                                     const gfx::FontList& font_list)
      : AutofillPopupChildView(controller, suggestion, line_number),
        label_(new views::Label(text, {font_list})) {
    views::BoxLayout* box_layout =
        new views::BoxLayout(views::BoxLayout::kVertical);
    box_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
    label_->SetBackground(views::CreateSolidBackground(background));
    label_->SetEnabledColor(text_color);
    label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    SetLayoutManager(box_layout);
    AddChildView(label_);
  }
  ~AutofillPopupTextOnlyView() override {}

 private:
  views::Label* label_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupTextOnlyView);
};

}  // namespace

AutofillPopupExpandedViewViews::AutofillPopupExpandedViewViews(
    AutofillPopupController* controller,
    views::Widget* parent_widget)
    : AutofillPopupBaseView(controller, parent_widget),
      controller_(controller) {
  views::BoxLayout* box_layout =
      new views::BoxLayout(views::BoxLayout::kVertical);
  box_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
  SetLayoutManager(box_layout);

  CreateChildViews();
  SetBackground(views::CreateThemedSolidBackground(
      this, ui::NativeTheme::kColorId_ResultsTableNormalBackground));
}

AutofillPopupExpandedViewViews::~AutofillPopupExpandedViewViews() {}

void AutofillPopupExpandedViewViews::Show() {
  DoShow();
}

void AutofillPopupExpandedViewViews::Hide() {
  // The controller is no longer valid after it hides us.
  controller_ = NULL;

  DoHide();
}

void AutofillPopupExpandedViewViews::OnSuggestionsChanged() {
  DoUpdateBoundsAndRedrawPopup();
}

void AutofillPopupExpandedViewViews::OnSelectedRowChanged(
    base::Optional<int> previous_row_selection,
    base::Optional<int> current_row_selection) {}

void AutofillPopupExpandedViewViews::CreateChildViews() {
  views_.clear();
  RemoveAllChildViews(true /* delete_children */);
  for (int i = 0; i < controller_->GetLineCount(); ++i) {
    views::View* child_view;
    if (controller_->GetSuggestionAt(i).frontend_id ==
        autofill::POPUP_ITEM_ID_SEPARATOR) {
      // Ignore separators for now.
      child_view = new AutofillPopupChildView(
          controller_, controller_->GetSuggestionAt(i), i);
    } else {
      child_view = new AutofillPopupTextOnlyView(
          controller_, i, controller_->GetSuggestionAt(i),
          controller_->GetElidedValueAt(i),
          GetNativeTheme()->GetSystemColor(
              controller_->layout_model().GetValueFontColorIDForRow(i)),
          GetNativeTheme()->GetSystemColor(
              controller_->GetBackgroundColorIDForRow(i)),
          controller_->layout_model().GetValueFontListForRow(i)

              );
    }
    views_.push_back(child_view);
    AddChildView(child_view);
  }
}

}  // namespace autofill
