// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_view_native_views.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include <algorithm>

namespace {
const int kPopupBorderThicknessDp = 1;
}

namespace autofill {

AutofillPopupChildNativeView::AutofillPopupChildNativeView(
    AutofillPopupController* controller,
    int line_number)
    : controller_(controller), line_number_(line_number) {
  is_separator_ = controller_->GetSuggestionAt(line_number_).frontend_id ==
                  autofill::POPUP_ITEM_ID_SEPARATOR;

  SetFocusBehavior(is_separator_ ? FocusBehavior::ALWAYS
                                 : FocusBehavior::NEVER);
  CreateContent();
}

void AutofillPopupChildNativeView::AcceptSelection() {
  controller_->AcceptSuggestion(line_number_);
}

void AutofillPopupChildNativeView::SetStyle(bool is_selected) {
  // Only content rows, not separators, can be highlighted/selected.
  if (is_separator_)
    return;

  SetBackground(views::CreateThemedSolidBackground(
      this, is_selected
                ? ui::NativeTheme::kColorId_ResultsTableSelectedBackground
                : ui::NativeTheme::kColorId_ResultsTableNormalBackground));
  first_line_->SetEnabledColor(GetNativeTheme()->GetSystemColor(
      is_selected ? ui::NativeTheme::kColorId_ResultsTableNormalText
                  : ui::NativeTheme::kColorId_ResultsTableSelectedText));

  if (second_line_) {
    second_line_->SetEnabledColor(GetNativeTheme()->GetSystemColor(
        is_selected
            ? ui::NativeTheme::kColorId_ResultsTableNormalDimmedText
            : ui::NativeTheme::kColorId_ResultsTableSelectedDimmedText));
  }
}

void AutofillPopupChildNativeView::OnMouseEntered(const ui::MouseEvent& event) {
  controller_->SetSelectedLine(line_number_);
}

void AutofillPopupChildNativeView::OnMouseReleased(
    const ui::MouseEvent& event) {
  if (is_separator_)
    return;

  if (event.IsOnlyLeftMouseButton() && HitTestPoint(event.location()))
    AcceptSelection();
  }

  bool AutofillPopupChildNativeView::OnMouseDragged(
      const ui::MouseEvent& event) {
    return true;
  }

  bool AutofillPopupChildNativeView::OnMousePressed(
      const ui::MouseEvent& event) {
    return true;
  }

  void AutofillPopupChildNativeView::GetAccessibleNodeData(
      ui::AXNodeData* node_data) {
    node_data->role = ui::AX_ROLE_MENU_ITEM;
    node_data->SetName(controller_->GetSuggestionAt(line_number_).value);
  }

  void AutofillPopupChildNativeView::CreateContent() {
    int horizontal_padding = is_separator_ ? 0 : 13;
    auto box_layout = std::make_unique<views::BoxLayout>(
        views::BoxLayout::kVertical, gfx::Insets(4, horizontal_padding));
    box_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
    SetLayoutManager(std::move(box_layout));

    if (is_separator_) {
      views::Separator* separator = new views::Separator();
      AddChildView(separator);
    } else {
      first_line_ = new views::Label(
          // TODO(tmartino): Remove elision responsibilities from controller.
          controller_->GetElidedValueAt(line_number_),
          // TODO(tmartino): Remove font list responsibilities from controller.
          {controller_->layout_model().GetValueFontListForRow(line_number_)});

      first_line_->SetEnabledColor(GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_ResultsTableNormalText));
      first_line_->SetSelectionTextColor(GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_ResultsTableSelectedText));
      // TODO(tmartino): Remove font color responsibilities from controller.
      // controller_->layout_model().GetValueFontColorIDForRow(line_number_)));
      first_line_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

      AddChildView(first_line_);

      const base::string16& description_text =
          controller_->GetElidedLabelAt(line_number_);
      if (description_text.empty()) {
        second_line_ = nullptr;
      } else {
        second_line_ = new views::Label(
            description_text,
            {controller_->layout_model().GetLabelFontListForRow(line_number_)});

        second_line_->SetEnabledColor(GetNativeTheme()->GetSystemColor(
            ui::NativeTheme::kColorId_ResultsTableNormalDimmedText));
        second_line_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

        AddChildView(second_line_);
      }
    }
  }

  AutofillPopupViewNativeViews::AutofillPopupViewNativeViews(
      AutofillPopupController* controller,
      views::Widget* parent_widget)
      : AutofillPopupBaseView(controller, parent_widget),
        controller_(controller) {
    auto box_layout =
        std::make_unique<views::BoxLayout>(views::BoxLayout::kVertical);
    box_layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
    SetLayoutManager(std::move(box_layout));

    CreateChildViews();
    SetBackground(views::CreateThemedSolidBackground(
        this, ui::NativeTheme::kColorId_ResultsTableNormalBackground));
}

AutofillPopupViewNativeViews::~AutofillPopupViewNativeViews() {}

void AutofillPopupViewNativeViews::Show() {
  DoShow();
}

void AutofillPopupViewNativeViews::Hide() {
  // The controller is no longer valid after it hides us.
  controller_ = nullptr;

  DoHide();
}

void AutofillPopupViewNativeViews::OnSelectedRowChanged(
    base::Optional<int> previous_row_selection,
    base::Optional<int> current_row_selection) {
  if (previous_row_selection) {
    AutofillPopupChildNativeView* previous = rows_[*previous_row_selection];
    if (previous)
      previous->SetStyle(false);
  }

  if (current_row_selection) {
    AutofillPopupChildNativeView* current = rows_[*current_row_selection];
    if (current)
      current->SetStyle(true);
  }
}

void AutofillPopupViewNativeViews::OnSuggestionsChanged() {
  DoUpdateBoundsAndRedrawPopup();
}

void AutofillPopupViewNativeViews::CreateChildViews() {
  RemoveAllChildViews(true /* delete_children */);
  for (int i = 0; i < controller_->GetLineCount(); ++i) {
    AutofillPopupChildNativeView* child_view =
        new AutofillPopupChildNativeView(controller_, i);
    rows_.push_back(child_view);
    AddChildView(child_view);
  }
}

gfx::Rect AutofillPopupViewNativeViews::GetUpdatedBounds() {
  // gfx::Rect bounds = AutofillPopupBaseView::GetUpdatedBounds();
  gfx::Rect bounds = GetBoundsFromDelegate();
  bounds.set_width(std::max(width(), bounds.width()));
  bounds.set_height(std::max(height(), bounds.height()));
  return bounds;
}

void AutofillPopupViewNativeViews::DoUpdateBoundsAndRedrawPopup() {
  gfx::Rect bounds = GetUpdatedBounds();
  SetSize(bounds.size());
  bounds.set_height(bounds.height() + 2 * kPopupBorderThicknessDp);
  bounds.set_width(bounds.width() + 2 * kPopupBorderThicknessDp);
  GetWidget()->SetBounds(bounds);
  SchedulePaint();
}

}  // namespace autofill
