// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/picture_in_picture/PictureInPictureController.h"

#include "core/dom/Document.h"
#include "platform/feature_policy/FeaturePolicy.h"

namespace blink {

PictureInPictureController::PictureInPictureController(Document& document)
    : Supplement<Document>(document), picture_in_picture_enabled_(true) {}

PictureInPictureController::~PictureInPictureController() = default;

// static
PictureInPictureController& PictureInPictureController::Ensure(
    Document& document) {
  PictureInPictureController* controller =
      static_cast<PictureInPictureController*>(
          Supplement<Document>::From(document, SupplementName()));
  if (!controller) {
    controller = new PictureInPictureController(document);
    ProvideTo(document, SupplementName(), controller);
  }
  return *controller;
}

// static
const char* PictureInPictureController::SupplementName() {
  return "PictureInPictureController";
}

bool PictureInPictureController::PictureInPictureEnabled() {
  DCHECK(GetSupplementable());

  // Return false if document is not allowed to use Picture-in-Picture
  // Feature Policy.
  LocalFrame* frame = GetSupplementable()->GetFrame();

  if (IsSupportedInFeaturePolicy(
          blink::FeaturePolicyFeature::kPictureInPicture) &&
      !frame->IsFeatureEnabled(
          blink::FeaturePolicyFeature::kPictureInPicture)) {
    return false;
  }

  // TODO: Return false if not Android O and onwards.
  // TODO: Return false if Android user has deactivated PiP for Chrome.

  return picture_in_picture_enabled_;
}

void PictureInPictureController::SetPictureInPictureEnabledForTesting(
    bool value) {
  picture_in_picture_enabled_ = value;
}

}  // namespace blink
