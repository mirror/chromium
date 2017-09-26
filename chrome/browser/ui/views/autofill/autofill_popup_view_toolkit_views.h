// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_TOOLKIT_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_TOOLKIT_VIEWS_H_

#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/ui/autofill/autofill_popup_view.h"
#include "chrome/browser/ui/views/autofill/autofill_popup_base_view.h"

namespace autofill {

class AutofillPopupController;

class AutofillPopupViewToolkitViews : public AutofillPopupBaseView,
                                      public AutofillPopupView {
 public:
  AutofillPopupViewToolkitViews(AutofillPopupController* controller,
                                views::Widget* parent_widget);
  void Show() override;
  void Hide() override;

 private:
  ~AutofillPopupViewToolkitViews() override;

  void OnSelectedRowChanged(base::Optional<int> previous_row_selection,
                            base::Optional<int> current_row_selection) override;
  void OnSuggestionsChanged() override;

  // Creates child views based on the suggestions given by |controller_|.
  void CreateChildViews();

  std::vector<views::View*> views_;

  // Controller for this view. Weak reference.
  AutofillPopupController* controller_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupViewToolkitViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_POPUP_VIEW_TOOLKIT_VIEWS_H_
