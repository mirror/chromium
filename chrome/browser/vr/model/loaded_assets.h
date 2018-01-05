// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_LOADED_ASSETS_H_
#define CHROME_BROWSER_VR_MODEL_LOADED_ASSETS_H_

#include <memory>

class SkBitmap;

namespace vr {

// TODO(vollick): ideally, this would be Assets and our current Assets could be
// named something like AssetsLoader, but using this name minimizes code churn.
struct LoadedAssets {
  LoadedAssets();
  ~LoadedAssets();
  // NB: this is required. This image (and the gradients below) are destroyed
  // after they are uploaded to the GPU. So while this image is never initially
  // null after a successful load, after the image has been consumed, it will
  // be.
  std::unique_ptr<SkBitmap> background;

  // NB: for backwards compatibility, we may not have these gradients in the
  // component. Receiving code must be able to cope with these gradient images
  // not existing.
  std::unique_ptr<SkBitmap> normal_gradient;
  std::unique_ptr<SkBitmap> incognito_gradient;
  std::unique_ptr<SkBitmap> fullscreen_gradient;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_LOADED_ASSETS_H_
