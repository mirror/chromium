// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/autofill_popup_view.h"

#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/views/autofill/autofill_popup_view_views.h"

namespace autofill {

AutofillPopupView* AutofillPopupView::Create(
    AutofillPopupController* controller) {
  return AutofillPopupViewViews::Create(controller);
}

}  // namespace autofill
