// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureWindow_h
#define PictureInPictureWindow_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

// The PictureInPictureWindow is meant to be used only by
// PictureInPictureController and is fundamentally just a simple proxy to get
// information such as dimensions about the current Picture-in-Picture window.
class PictureInPictureWindow : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PictureInPictureWindow* Create(int width, int height);

  int width() const { return width_; }
  int height() const { return height_; }

  // Called when Picture-in-Picture window state is closed.
  void SetNullDimensions();

 private:
  PictureInPictureWindow(int width, int height);

  // The Picture-in-Picture window width in pixels.
  int width_;

  // The Picture-in-Picture window height in pixels.
  int height_;
};

}  // namespace blink

#endif  // PictureInPictureWindow_h
