// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DocumentOrShadowRootPictureInPicture_h
#define DocumentOrShadowRootPictureInPicture_h

#include "platform/heap/Handle.h"

namespace blink {

class Document;
class HTMLVideoElement;

class DocumentOrShadowRootPictureInPicture {
  STATIC_ONLY(DocumentOrShadowRootPictureInPicture);

 public:
  static HTMLVideoElement* pictureInPictureElement(Document&);
};

}  // namespace blink

#endif  // DocumentOrShadowRootPictureInPicture_h
