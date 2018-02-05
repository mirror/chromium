// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureWindow_h
#define PictureInPictureWindow_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class Document;

class PictureInPictureWindow : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PictureInPictureWindow* Create(const Document&, int width, int height);

  int width() const { return width_; }
  int height() const { return height_; }

 private:
  PictureInPictureWindow(const Document&, int width, int height);

  int width_;
  int height_;
};

}  // namespace blink

#endif  // PictureInPictureWindow_h
