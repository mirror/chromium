// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PictureInPictureElements_h
#define PictureInPictureElements_h

#include "core/html/media/PictureInPictureInterstitial.h"

namespace blink {

// A text message shown to indicate picture in picture is active.
class PictureInPictureMessageElement final : public HTMLDivElement {
 public:
  explicit PictureInPictureMessageElement(PictureInPictureInterstitial&);
};

}  // namespace blink

#endif  // PictureInPictureElements_h