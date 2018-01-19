// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_NATIVE_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_NATIVE_VIEWS_H_

#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/browser/ui/views/autofill/autofill_popup_base_view.h"

#include <vector>

namespace views {
class Label;
}

namespace autofill {

class AutofillPopupController;

// Child view representing one row (i.e., one suggestion) in the Autofill
// Popup.
class AutofillPopupChildNativeView : public views::View {
 public:
  AutofillPopupChildNativeView(AutofillPopupController* controller,
                               int line_number);

  ~AutofillPopupChildNativeView() override {}

  void AcceptSelection();
  void SetStyle(bool is_selected);

  // views::View:
  // TODO(tmartino): Consolidate and deprecate code in AutofillPopupBaseView
  // where overlap exists with these events.
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;

  bool OnMouseDragged(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

 private:
  // views::View:
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void CreateContent();

  AutofillPopupController* controller_;
  views::Label* first_line_;
  views::Label* second_line_;
  int line_number_;
  bool is_separator_;
  bool is_warning_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupChildNativeView);
};

// Views implementation for the autofill and password suggestion.
// TODO(https://crbug.com/768881): Once this implementation is complete, this
// class should be renamed to AutofillPopupViewViews and old
// AutofillPopupViewViews should be removed. The main difference of
// AutofillPopupViewNativeViews from AutofillPopupViewViews is that child views
// are drawn using toolkit-views framework, in contrast to
// AutofillPopupViewViews, where individuals rows are drawn directly on canvas.
class AutofillPopupViewNativeViews : public AutofillPopupBaseView,
                                     public AutofillPopupView {
 public:
  AutofillPopupViewNativeViews(AutofillPopupController* controller,
                               views::Widget* parent_widget);
  ~AutofillPopupViewNativeViews() override;

  void Show() override;
  void Hide() override;

 private:
  void OnSelectedRowChanged(base::Optional<int> previous_row_selection,
                            base::Optional<int> current_row_selection) override;
  void OnSuggestionsChanged() override;

  // Creates child views based on the suggestions given by |controller_|.
  void CreateChildViews();

  // AutofillPopupBaseView:
  gfx::Rect GetUpdatedBounds() override;
  void DoUpdateBoundsAndRedrawPopup() override;

  // Controller for this view.
  AutofillPopupController* controller_;

  std::vector<AutofillPopupChildNativeView*> rows_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupViewNativeViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_NATIVE_VIEWS_H_
