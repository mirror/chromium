// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/native_theme/native_theme_aura.h"

namespace ui {

// static
NativeTheme* NativeTheme::GetInstanceForNativeUi() {
  return NativeThemeAura::instance();
}

}  // namespace ui
