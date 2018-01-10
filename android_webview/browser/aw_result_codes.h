// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_RESULT_CODES_H_
#define ANDROID_WEBVIEW_BROWSER_AW_RESULT_CODES_H_

#include "content/public/common/content_result_codes.h"

namespace android_webview {

enum ResultCode {
  // Inherited from content::ResultCode:
  RESULT_CODE_NORMAL_EXIT,
  RESULT_CODE_KILLED,
  RESULT_CODE_HUNG,
  RESULT_CODE_KILLED_BAD_MESSAGE,
  RESULT_CODE_GPU_DEAD_ON_ARRIVAL,

  // First android_webview::ResultCode
  RESULT_CODE_START_CODE,

  // A critical resource file is missing.
  RESULT_CODE_MISSING_DATA = RESULT_CODE_START_CODE,

  // Last android_webview::ResultCode (keep this last).
  RESULT_CODE_LAST_CODE,
};

}  // namespace android_webview

static_assert(static_cast<int>(android_webview::RESULT_CODE_NORMAL_EXIT) ==
                  static_cast<int>(content::RESULT_CODE_NORMAL_EXIT),
              "RESULT_CODE_NORMAL_EXIT mismatch");

static_assert(static_cast<int>(android_webview::RESULT_CODE_KILLED) ==
                  static_cast<int>(content::RESULT_CODE_KILLED),
              "RESULT_CODE_KILLED mismatch");

static_assert(static_cast<int>(android_webview::RESULT_CODE_HUNG) ==
                  static_cast<int>(content::RESULT_CODE_HUNG),
              "RESULT_CODE_HUNG mismatch");

static_assert(
    static_cast<int>(android_webview::RESULT_CODE_KILLED_BAD_MESSAGE) ==
        static_cast<int>(content::RESULT_CODE_KILLED_BAD_MESSAGE),
    "RESULT_CODE_KILLED_BAD_MESSAGE mismatch");

static_assert(
    static_cast<int>(android_webview::RESULT_CODE_GPU_DEAD_ON_ARRIVAL) ==
        static_cast<int>(content::RESULT_CODE_GPU_DEAD_ON_ARRIVAL),
    "RESULT_CODE_GPU_DEAD_ON_ARRIVAL mismatch");

static_assert(static_cast<int>(android_webview::RESULT_CODE_START_CODE) ==
                  static_cast<int>(content::RESULT_CODE_LAST_CODE),
              "RESULT_CODE_START_CODE vs LAST_CODE mismatch");

#endif  // ANDROID_WEBVIEW_BROWSER_AW_RESULT_CODES_H_
