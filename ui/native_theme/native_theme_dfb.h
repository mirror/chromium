// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_NATIVE_THEME_NATIVE_THEME_DFB_H_
#define UI_NATIVE_THEME_NATIVE_THEME_DFB_H_

#include "ui/native_theme/native_theme_base.h"

namespace ui {

// Dfb implementation of native theme support.
class NativeThemeDfb : public NativeThemeBase {
 public:
  static NativeThemeDfb* instance();

  virtual SkColor GetSystemColor(ColorId color_id) const OVERRIDE;

 private:
  NativeThemeDfb();
  virtual ~NativeThemeDfb();

  DISALLOW_COPY_AND_ASSIGN(NativeThemeDfb);
};

}  // namespace ui

#endif  // UI_NATIVE_THEME_NATIVE_THEME_DFB_H_
