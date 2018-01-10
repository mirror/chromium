// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_CONTENT_RESULT_CODES_H_
#define CONTENT_PUBLIC_COMMON_CONTENT_RESULT_CODES_H_

#include "base/result_codes.h"

namespace content {

// This file adds the return codes for the browser and renderer process
// to the ones provided by base/result_codes.h.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content_public.common
enum ResultCode {
  // Inherited from base::ResultCode:
  RESULT_CODE_NORMAL_EXIT,
  RESULT_CODE_KILLED,
  RESULT_CODE_HUNG,

  // First content::ResultCode
  RESULT_CODE_START_CODE,

  // A bad message caused the process termination.
  RESULT_CODE_KILLED_BAD_MESSAGE = RESULT_CODE_START_CODE,

  // The GPU process exited because initialization failed.
  RESULT_CODE_GPU_DEAD_ON_ARRIVAL,

  // Last content::ResultCode (keep this last).
  RESULT_CODE_LAST_CODE
};

}  // namespace content

static_assert(static_cast<int>(content::RESULT_CODE_NORMAL_EXIT) ==
                  static_cast<int>(base::RESULT_CODE_NORMAL_EXIT),
              "RESULT_CODE_NORMAL_EXIT mismatch");

static_assert(static_cast<int>(content::RESULT_CODE_KILLED) ==
                  static_cast<int>(base::RESULT_CODE_KILLED),
              "RESULT_CODE_KILLED mismatch");

static_assert(static_cast<int>(content::RESULT_CODE_HUNG) ==
                  static_cast<int>(base::RESULT_CODE_HUNG),
              "RESULT_CODE_HUNG mismatch");

static_assert(static_cast<int>(content::RESULT_CODE_START_CODE) ==
                  static_cast<int>(base::RESULT_CODE_LAST_CODE),
              "RESULT_CODE_START_CODE vs LAST_CODE  mismatch");

#endif  // CONTENT_PUBLIC_COMMON_CONTENT_RESULT_CODES_H_
