// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/HTMLVideoElementPictureInPicture.h"

#include "core/dom/DOMException.h"
#include "core/html/media/HTMLVideoElement.h"
#include "modules/picture_in_picture/PictureInPictureController.h"
#include "platform/feature_policy/FeaturePolicy.h"

namespace blink {
namespace {

const char kFeaturePolicyBlocked[] =
    "Access to the feature \"picture-in-picture\" is disallowed by feature "
    "policy.";
const char kNotAvailable[] = "Picture-in-Picture is not available.";
const char kUserGestureRequired[] =
    "Must be handling a user gesture to request picture in picture.";
}  // namespace

// static
ScriptPromise HTMLVideoElementPictureInPicture::requestPictureInPicture(
    ScriptState* script_state,
    HTMLVideoElement& element) {
  // If document is not allowed to use the policy-controlled feature named
  // "picture-in-picture", reject promise with a SecurityError and abort these
  //  steps.
  LocalFrame* frame = element.GetFrame();
  if (IsSupportedInFeaturePolicy(
          blink::FeaturePolicyFeature::kPictureInPicture) &&
      !frame->IsFeatureEnabled(
          blink::FeaturePolicyFeature::kPictureInPicture)) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kSecurityError, kFeaturePolicyBlocked));
  }

  // If pictureInPictureEnabled is false, reject promise with a
  // NotSupportedError and abort these steps.
  Document& document = element.GetDocument();
  if (!PictureInPictureController::Ensure(document).PictureInPictureEnabled()) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kNotSupportedError, kNotAvailable));
  }

  // If the algorithm is not triggered by user activation, reject promise with a
  // NotAllowedError and abort these steps.
  if (!Frame::ConsumeTransientUserActivation(frame)) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kNotAllowedError, kUserGestureRequired));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO: Call element.enterPictureInPicture() when implemented.

  resolver->Resolve();
  return promise;
}

}  // namespace blink
