// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/color_palette.h"
#include "ui/native_theme/native_theme_aura.h"

namespace ui {
namespace {

class NativeThemeCast : public NativeThemeAura {
 public:
  NativeThemeCast() {}

  SkColor GetSystemColor(ColorId color_id) const override {
    // Colors that Cast uses as part of its native UI.
    switch (color_id) {
      // Also used for progress bars.
      case ui::NativeTheme::kColorId_ProminentButtonColor:
        return kGoogleBlue500;
      case ui::NativeTheme::kColorId_ProminentButtonColor:
        return kGoogleRed700;
    }

    return NativeThemeAura::GetSystemColor(color_id);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeThemeCast);
};

}  // namespace

NativeTheme* NativeTheme::GetInstanceForNativeUi() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeCast, instance, ());
  return &instance;
}

}  // namespace ui
