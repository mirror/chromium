// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/splash_screen_icon.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/elements/splash_screen_text_texture.h"

namespace vr {

SplashScreenText::SplashScreenText(int preferred_width)
    : TexturedElement(preferred_width),
      texture_(base::MakeUnique<SplashScreenTextTexture>()) {}

SplashScreenText::~SplashScreenText() = default;

void SplashScreenText::SetSplashScreenTextBitmap(const SkBitmap& bitmap) {
  texture_->SetSplashScreenTextBitmap(bitmap);
  UpdateTexture();
}

UiTexture* SplashScreenText::GetTexture() const {
  return texture_.get();
}

}  // namespace vr
