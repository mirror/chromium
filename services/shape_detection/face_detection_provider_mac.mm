// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_provider_mac.h"

#include "services/shape_detection/face_detection_impl_mac.h"
#if defined(MAC_OS_X_VERSION_10_13) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_13
#define VISION_FRAMEWORK_AVAILABLE
#include "services/shape_detection/face_detection_impl_mac_vision.h"
#endif

namespace shape_detection {

FaceDetectionProviderMac::FaceDetectionProviderMac() = default;
FaceDetectionProviderMac::~FaceDetectionProviderMac() = default;

void FaceDetectionProviderMac::CreateFaceDetection(
    mojom::FaceDetectionRequest request,
    mojom::FaceDetectorOptionsPtr options) {
#if defined(VISION_FRAMEWORK_AVAILABLE)
  // Vision Framework needs at least MAC OS X 10.13.
  if (@available(macOS 10.13, *)) {
    mojo::MakeStrongBinding(std::make_unique<FaceDetectionImplMacVision>(),
                            std::move(request));
  } else {
#endif
    mojo::MakeStrongBinding(
        std::make_unique<FaceDetectionImplMac>(std::move(options)),
        std::move(request));
#if defined(VISION_FRAMEWORK_AVAILABLE)
  }
#endif
}

}  // namespace shape_detection
