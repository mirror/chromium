// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/autofill_popup_view_toolkit_views.h"

#include "base/strings/string16.h"
#include "chrome/app/vector_icons/vector_icons.h"
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
#include "ui/views/background.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace autofill {

namespace {

class AutofillPopupChildView : public views::View {
 public:
  explicit AutofillPopupChildView(const Suggestion& suggestion)
      : suggestion_(suggestion) {
    SetFocusBehavior(FocusBehavior::ALWAYS);
  }

  ~AutofillPopupChildView() override {}

 private:
  // views::Views implementation
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override {
    node_data->role = ui::AX_ROLE_MENU_ITEM;
    node_data->SetName(suggestion_.value);
  }

  const Suggestion& suggestion_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupChildView);
};

class AutofillPopupTextOnlyView : public AutofillPopupChildView {
 public:
  explicit AutofillPopupTextOnlyView(AutofillPopupController* controller,
                                     const Suggestion& suggestion,
                                     const base::string16& text,
                                     SkColor text_color,
                                     const gfx::FontList& font_list)
      : AutofillPopupChildView(suggestion), label_(new views::Label(text)) {
    SetLayoutManager(new views::FillLayout());
    AddChildView(label_);
  }
  ~AutofillPopupTextOnlyView() override {}

 private:
  views::Label* label_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupTextOnlyView);
};

}  // namespace

AutofillPopupViewToolkitViews::AutofillPopupViewToolkitViews(
    AutofillPopupController* controller,
    views::Widget* parent_widget)
    : AutofillPopupBaseView(controller, parent_widget),
      controller_(controller) {
  SetLayoutManager(new views::FillLayout());
  CreateChildViews();
  SetBackground(views::CreateThemedSolidBackground(
      this, ui::NativeTheme::kColorId_ResultsTableNormalBackground));
}

AutofillPopupViewToolkitViews::~AutofillPopupViewToolkitViews() {}

void AutofillPopupViewToolkitViews::Show() {
  DoShow();
}

void AutofillPopupViewToolkitViews::Hide() {
  // The controller is no longer valid after it hides us.
  controller_ = NULL;

  DoHide();
}

void AutofillPopupViewToolkitViews::OnSuggestionsChanged() {
  DoUpdateBoundsAndRedrawPopup();
}

void AutofillPopupViewToolkitViews::OnSelectedRowChanged(
    base::Optional<int> previous_row_selection,
    base::Optional<int> current_row_selection) {}

void AutofillPopupViewToolkitViews::CreateChildViews() {
  views_.clear();
  RemoveAllChildViews(true /* delete_children */);
  for (int i = 0; i < controller_->GetLineCount(); ++i) {
    auto* view = new AutofillPopupTextOnlyView(
        controller_, controller_->GetSuggestionAt(i),
        controller_->GetElidedValueAt(i),
        GetNativeTheme()->GetSystemColor(
            controller_->layout_model().GetValueFontColorIDForRow(i)),
        controller_->layout_model().GetValueFontListForRow(i));
    views_.push_back(view);
    AddChildView(view);
  }
}

}  // namespace autofill
