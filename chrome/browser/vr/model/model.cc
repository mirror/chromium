// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/model/model.h"

namespace vr {

Model::Model() {}

Model::~Model() {}

const ColorScheme& Model::color_scheme() const {
  ColorScheme::Mode mode = ColorScheme::kModeNormal;
  if (incognito)
    mode = ColorScheme::kModeIncognito;
  if (fullscreen)
    mode = ColorScheme::kModeFullscreen;
  return ColorScheme::GetColorScheme(mode);
}

}  // namespace vr
