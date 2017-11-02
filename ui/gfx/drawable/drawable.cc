// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/drawable/drawable.h"

namespace gfx {

DrawableSource::DrawableSource(const gfx::Size& size) : size_(size) {}

DrawableSource::~DrawableSource() {}

const SkBitmap* DrawableSource::GetBitmap() const {
  return nullptr;
}

}  // namespace gfx
