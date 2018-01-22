// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/DocumentPictureInPicture.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/frame/LocalFrame.h"
#include "platform/feature_policy/FeaturePolicy.h"

namespace blink {

bool DocumentPictureInPicture::pictureInPictureEnabled(
    const Document& document) {
  LocalFrame* frame = document.GetFrame();
  if (IsSupportedInFeaturePolicy(
          blink::FeaturePolicyFeature::kPictureInPicture) &&
      !frame->IsFeatureEnabled(
          blink::FeaturePolicyFeature::kPictureInPicture)) {
    return false;
  }

  // TODO: Return false if not Android O and onwards.

  // TODO: Return false if Android user has desactivated PiP for Chrome.

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
