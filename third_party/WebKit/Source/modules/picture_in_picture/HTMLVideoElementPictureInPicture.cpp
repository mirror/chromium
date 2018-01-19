// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/HTMLVideoElementPictureInPicture.h"

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/frame/Frame.h"
#include "core/frame/LocalFrame.h"
#include "core/html/media/HTMLVideoElement.h"
#include "platform/feature_policy/FeaturePolicy.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"

namespace blink {
namespace {

const char kFeaturePolicyBlocked[] =
    "Access to the feature \"picture-in-picture\" is disallowed by feature "
    "policy.";
const char kUserGestureRequired[] =
    "Must be handling a user gesture to request picture in picture.";
}  // namespace

// static
ScriptPromise HTMLVideoElementPictureInPicture::requestPictureInPicture(
    ScriptState* script_state,
    HTMLVideoElement& element) {
  LocalFrame* frame = element.GetFrame();

  if (!frame) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kNotSupportedError));
  }

  // If document is not allowed to use the policy-controlled feature named
  // "picture-in-picture", reject promise with a SecurityError and abort these
  //  steps.
  if (IsSupportedInFeaturePolicy(
          blink::FeaturePolicyFeature::kPictureInPicture) &&
      !frame->IsFeatureEnabled(
          blink::FeaturePolicyFeature::kPictureInPicture)) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kSecurityError, kFeaturePolicyBlocked));
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
