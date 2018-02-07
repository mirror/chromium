// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/PictureInPictureWindow.h"

#include "core/dom/Document.h"

namespace blink {

PictureInPictureWindow* PictureInPictureWindow::Create(int width, int height) {
  return new PictureInPictureWindow(width, height);
}

PictureInPictureWindow::PictureInPictureWindow(int width, int height)
    : width_(width), height_(height) {}

void PictureInPictureWindow::SetNullDimensions() {
  width_ = 0;
  height_ = 0;
}

}  // namespace blink
