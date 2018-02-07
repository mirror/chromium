// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_OVERSCROLL_CONFIGURATION_H_
#define CONTENT_PUBLIC_BROWSER_OVERSCROLL_CONFIGURATION_H_

#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT OverscrollConfig {
 public:
  // Determines overscroll history navigation mode according to its
  // corresponding flag.
  enum class Mode {
    kDisabled,
    kEnabled,
    kSimple,
  };

  // Specifies an overscroll controller threshold.
  enum class Threshold {
    // Threshold to complete touchpad overscroll, in terms of the percentage of
    // the display size.
    kCompleteTouchpad,

    // Threshold to complete touchscreen overscroll, in terms of the percentage
    // of
    // the display size.
    kCompleteTouchscreen,

    // Threshold to start touchpad overscroll, in DIPs.
    kStartTouchpad,

    // Threshold to start touchscreen overscroll, in DIPs.
    kStartTouchscreen,
  };

  static Mode GetMode();
  static bool IsEnabled();
  static bool IsScreenshotUi();

  static float GetThreshold(Threshold threshold);

 private:
  static bool is_mode_initialized_;
  static Mode mode_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(OverscrollConfig);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_OVERSCROLL_CONFIGURATION_H_
