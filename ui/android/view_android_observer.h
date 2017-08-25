// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_VIEW_ANDROID_OBSERVER_H_
#define UI_ANDROID_VIEW_ANDROID_OBSERVER_H_

#include "ui/android/ui_android_export.h"

namespace ui {

class UI_ANDROID_EXPORT ViewAndroidObserver {
 public:
  virtual void OnAttachedToWindow() = 0;
  virtual void OnDetachedFromWindow() = 0;

 protected:
  virtual ~ViewAndroidObserver() {}
};

}  // namespace ui

#endif  // UI_ANDROID_VIEW_ANDROID_OBSERVER_H_
