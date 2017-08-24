// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_WINDOW_ANDROID_MONITOR_H_
#define UI_ANDROID_WINDOW_ANDROID_MONITOR_H_

#include "ui/android/ui_android_export.h"

namespace ui {

class UI_ANDROID_EXPORT WindowAndroidMonitor {
 public:
  virtual void OnAttachedToWindow() = 0;
  virtual void OnDetachedFromWindow() = 0;

 protected:
  virtual ~WindowAndroidMonitor() {}
};

}  // namespace ui

#endif  // UI_ANDROID_WINDOW_ANDROID_MONITOR_H_
