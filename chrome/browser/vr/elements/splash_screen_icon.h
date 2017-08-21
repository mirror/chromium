// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_SPLASH_SCREEN_ICON_H_
#define CHROME_BROWSER_VR_ELEMENTS_SPLASH_SCREEN_ICON_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/vr/elements/textured_element.h"

class SkBitmap;

namespace vr {

class SplashScreenTextTexture;

class SplashScreenText : public TexturedElement {
 public:
  explicit SplashScreenText(int preferred_width);
  ~SplashScreenText() override;

  void SetSplashScreenTextBitmap(const SkBitmap& bitmap);

 private:
  UiTexture* GetTexture() const override;

  std::unique_ptr<SplashScreenTextTexture> texture_;

  DISALLOW_COPY_AND_ASSIGN(SplashScreenText);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_SPLASH_SCREEN_ICON_H_
