// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/ShadowRootPictureInPicture.h"

#include "core/dom/Document.h"
#include "core/html/media/HTMLVideoElement.h"
#include "modules/picture_in_picture/PictureInPictureController.h"

namespace blink {

// static
HTMLVideoElement* ShadowRootPictureInPicture::pictureInPictureElement(
    TreeScope& scope) {
  LOG(INFO) << "ShadowRootPictureInPicture::pictureInPictureElement";
  return PictureInPictureController::Ensure(scope.GetDocument())
      .PictureInPictureElement(scope);
}

}  // namespace blink
