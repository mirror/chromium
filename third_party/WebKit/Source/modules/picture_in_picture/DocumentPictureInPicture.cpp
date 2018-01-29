// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/DocumentPictureInPicture.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "modules/picture_in_picture/PictureInPictureController.h"

namespace blink {

// static
bool DocumentPictureInPicture::pictureInPictureEnabled(Document& document) {
  return PictureInPictureController::Ensure(document).PictureInPictureEnabled();
}

// static
ScriptPromise DocumentPictureInPicture::exitPictureInPicture(
    ScriptState* script_state,
    Document& document) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO(crbug.com/806249): Call element.exitPictureInPicture().

  PictureInPictureController::Ensure(document).UnsetPictureInPictureElement();

  resolver->Reject(
      DOMException::Create(kNotSupportedError, "Not implemented yet"));
  return promise;
}

// static
HTMLVideoElement* DocumentPictureInPicture::pictureInPictureElement(
    Document& document) {
  return PictureInPictureController::Ensure(document).PictureInPictureElement();
}

}  // namespace blink
