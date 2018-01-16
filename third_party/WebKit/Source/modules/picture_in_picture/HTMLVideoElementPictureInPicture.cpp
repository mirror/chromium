// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/HTMLVideoElementPictureInPicture.h"

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/html/media/HTMLVideoElement.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"

namespace blink {

// static
ScriptPromise HTMLVideoElementPictureInPicture::requestPictureInPicture(
    ScriptState* script_state,
    HTMLVideoElement& element) {
  ScriptPromiseResolver* resolver_ =
      ScriptPromiseResolver::Create(script_state);
  return resolver_->Promise();
}

}  // namespace blink
