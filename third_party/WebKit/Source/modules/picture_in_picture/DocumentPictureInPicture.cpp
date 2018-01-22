// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/DocumentPictureInPicture.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"

namespace blink {

bool DocumentPictureInPicture::pictureInPictureEnabled(const Document&) {
  // TODO: Implement this
  return true;
}

ScriptPromise DocumentPictureInPicture::exitPictureInPicture(
    ScriptState* script_state,
    const Document&) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO: Call element.exitPictureInPicture() when implemented.

  resolver->Resolve();
  return promise;
}

}  // namespace blink
