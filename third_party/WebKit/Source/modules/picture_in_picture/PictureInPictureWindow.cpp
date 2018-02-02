// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/PictureInPictureWindow.h"

#include "core/dom/Document.h"

namespace blink {

PictureInPictureWindow* PictureInPictureWindow::Create(const Document& document,
                                                       unsigned width,
                                                       unsigned height) {
  return new PictureInPictureWindow(document, width, height);
}

PictureInPictureWindow::PictureInPictureWindow(const Document& document,
                                               unsigned width,
                                               unsigned height)
    : width_(width), height_(height) {}
