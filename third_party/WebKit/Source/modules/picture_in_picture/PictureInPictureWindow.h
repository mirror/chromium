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
  static PictureInPictureWindow* Create(const Document&,
                                        unsigned width,
                                        unsigned height);

  unsigned width() const { return width_; }
  unsigned height() const { return height_; }

 private:
  PictureInPictureWindow(const Document&, unsigned width, unsigned height);

  unsigned width_;
  unsigned height_;

}  // namespace blink

#endif  // PictureInPictureWindow_h
