// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_mac_vision.h"

namespace shape_detection {

FaceDetectionImplMacVision::FaceDetectionImplMacVision() {
  landmarks_request_.reset([[VNDetectFaceLandmarksRequest alloc] init]);
}

FaceDetectionImplMacVision::~FaceDetectionImplMacVision() = default;

void FaceDetectionImplMacVision::Detect(const SkBitmap& bitmap,
                                        DetectCallback callback) {
  std::move(callback).Run({});
}

}  // namespace shape_detection
