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
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include <algorithm>

namespace {

// Thickness of the outside border which is added by the base view. Our size
// needs to be adjusted to accommodate this border.
const int kPopupBorderThicknessDp = 1;

// Padding values for the entire dropdown.
const int kPopupTopBottomPadding = 8;
const int kPopupSidePadding = 0;

// Padding values and dimensions for rows.
const int kRowHeight = 28;
const int kRowTopBottomPadding = 0;
const int kRowSidePadding = 16;

// Padding values specific to the separator row.
const int kSeparatorTopBottomPadding = 4;
const int kSeparatorSidePadding = 0;

// By spec, dropdowns should have a min width of 64, and should always have
// a width which is a multiple of 16.
const int kDropdownSpecMultiple = 16;
const int kDropdownSpecMinWidth = 64;

int RoundWidthToSpec(int width) {
  width = width + (kDropdownSpecMultiple - (kDropdownSpecMultiple % width));
  return std::max(kDropdownSpecMinWidth, width);
}

}  // namespace

namespace autofill {

AutofillPopupRowView::AutofillPopupRowView(AutofillPopupController* controller,
                                           int line_number)
    : controller_(controller), line_number_(line_number) {
  int frontend_id = controller_->GetSuggestionAt(line_number_).frontend_id;
  is_separator_ = frontend_id == autofill::POPUP_ITEM_ID_SEPARATOR;
  is_warning_ =
      frontend_id == autofill::POPUP_ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE ||
      frontend_id ==
          autofill::POPUP_ITEM_ID_INSECURE_CONTEXT_PAYMENT_DISABLED_MESSAGE;

  SetFocusBehavior(is_separator_ ? FocusBehavior::ALWAYS
                                 : FocusBehavior::NEVER);
  CreateContent();
}

void AutofillPopupRowView::AcceptSelection() {
  controller_->AcceptSuggestion(line_number_);
}

void AutofillPopupRowView::SetStyle(bool is_selected) {
  // Only content rows, not separators, can be highlighted/selected.
  if (is_separator_)
    return;

  SetBackground(views::CreateThemedSolidBackground(
      this, is_selected
                ? ui::NativeTheme::kColorId_ResultsTableSelectedBackground
                : ui::NativeTheme::kColorId_ResultsTableNormalBackground));
}

void AutofillPopupRowView::OnMouseEntered(const ui::MouseEvent& event) {
  controller_->SetSelectedLine(line_number_);
}

void AutofillPopupRowView::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_separator_)
    return;

  if (event.IsOnlyLeftMouseButton() && HitTestPoint(event.location()))
    AcceptSelection();
  }

  bool AutofillPopupRowView::OnMouseDragged(const ui::MouseEvent& event) {
    return true;
  }

  bool AutofillPopupRowView::OnMousePressed(const ui::MouseEvent& event) {
    return true;
  }

  void AutofillPopupRowView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
    node_data->role = ui::AX_ROLE_MENU_ITEM;
    node_data->SetName(controller_->GetSuggestionAt(line_number_).value);
  }

  void AutofillPopupRowView::CreateContent() {
    if (is_separator_) {
      views::BoxLayout* layout =
          SetLayoutManager(std::make_unique<views::BoxLayout>(
              views::BoxLayout::kVertical,
              gfx::Insets(kSeparatorTopBottomPadding, kSeparatorSidePadding)));
      layout->set_main_axis_alignment(
          views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);
      AddChildView(new views::Separator());
      return;
    }

    SetBorder(views::CreateEmptyBorder(kRowTopBottomPadding, kRowSidePadding,
                                       kRowTopBottomPadding, kRowSidePadding));

    views::GridLayout* grid_layout =
        SetLayoutManager(std::make_unique<views::GridLayout>(this));
    views::ColumnSet* column_set = grid_layout->AddColumnSet(0);
    column_set->AddColumn(views::GridLayout::Alignment::LEADING,
                          views::GridLayout::Alignment::CENTER, 1,
                          views::GridLayout::SizeType::USE_PREF, 0, 0);
    column_set->AddPaddingColumn(1.0f, kRowSidePadding);
    column_set->AddColumn(views::GridLayout::Alignment::TRAILING,
                          views::GridLayout::Alignment::CENTER, 1,
                          views::GridLayout::SizeType::USE_PREF, 0, 0);

    grid_layout->StartRow(0, 0, kRowHeight);

    // TODO(tmartino): Remove elision, font list, and font color
    // responsibilities from controller.
    views::Label* text_label = new views::Label(
        controller_->GetElidedValueAt(line_number_),
        {controller_->layout_model().GetValueFontListForRow(line_number_)});

    ui::NativeTheme::ColorId primary_font_color =
        is_warning_ ? ui::NativeTheme::kColorId_ResultsTableNegativeText
                    : ui::NativeTheme::kColorId_ResultsTableNormalText;
    ui::NativeTheme::ColorId selected_font_color =
        is_warning_ ? ui::NativeTheme::kColorId_ResultsTableNegativeSelectedText
                    : ui::NativeTheme::kColorId_ResultsTableSelectedText;

    text_label->SetEnabledColor(
        GetNativeTheme()->GetSystemColor(primary_font_color));
    text_label->SetSelectionTextColor(
        GetNativeTheme()->GetSystemColor(selected_font_color));

    grid_layout->AddView(text_label);

    const base::string16& description_text =
        controller_->GetElidedLabelAt(line_number_);
    if (!description_text.empty()) {
      views::Label* subtext_label = new views::Label(
          description_text,
          {controller_->layout_model().GetLabelFontListForRow(line_number_)});

      subtext_label->SetEnabledColor(GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_ResultsTableNormalDimmedText));

      grid_layout->AddView(subtext_label);
    }
  }

  AutofillPopupViewNativeViews::AutofillPopupViewNativeViews(
      AutofillPopupController* controller,
      views::Widget* parent_widget)
      : AutofillPopupBaseView(controller, parent_widget),
        controller_(controller) {
    views::BoxLayout* layout =
        SetLayoutManager(std::make_unique<views::BoxLayout>(
            views::BoxLayout::kVertical,
            gfx::Insets(kPopupTopBottomPadding, kPopupSidePadding)));
    layout->set_main_axis_alignment(
        views::BoxLayout::MAIN_AXIS_ALIGNMENT_START);

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
    AutofillPopupRowView* previous = rows_[*previous_row_selection];
    if (previous)
      previous->SetStyle(false);
  }

  if (current_row_selection) {
    AutofillPopupRowView* current = rows_[*current_row_selection];
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
    AutofillPopupRowView* child_view = new AutofillPopupRowView(controller_, i);
    rows_.push_back(child_view);
    AddChildView(child_view);
  }
}

gfx::Rect AutofillPopupViewNativeViews::GetUpdatedBounds() {
  gfx::Rect bounds = GetBoundsFromDelegate();
  bounds.set_width(RoundWidthToSpec(width()));
  bounds.set_height(height());
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
