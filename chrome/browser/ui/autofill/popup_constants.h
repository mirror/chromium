// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AUTOFILL_POPUP_CONSTANTS_H_
#define CHROME_BROWSER_UI_AUTOFILL_POPUP_CONSTANTS_H_

#include "build/build_config.h"

namespace autofill {

#if defined(OS_MACOSX) || defined(OS_ANDROID)
// TODO(crbug.com/676221): Change this to pixels
const int kPopupBorderThickness = 1;
#else
// In views, the implementation takes care of the border itself.
const int kPopupBorderThickness = 0;
#endif

// The maximum amount of rows before popup becomes scrollable.
const size_t kPopupMaxVisibleRows = 10;

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_AUTOFILL_POPUP_CONSTANTS_H_
