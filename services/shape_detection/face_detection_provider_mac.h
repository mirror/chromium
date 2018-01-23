// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_MAC_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_MAC_H_

#include <memory>
#include <utility>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/public/interfaces/facedetection_provider.mojom.h"

namespace shape_detection {

class FaceDetectionProviderMac
    : public shape_detection::mojom::FaceDetectionProvider {
 public:
  FaceDetectionProviderMac();
  ~FaceDetectionProviderMac() override;

  static void Create(mojom::FaceDetectionProviderRequest request) {
    mojo::MakeStrongBinding(std::make_unique<FaceDetectionProviderMac>(),
                            std::move(request));
  }

  void CreateFaceDetection(mojom::FaceDetectionRequest request,
                           mojom::FaceDetectorOptionsPtr options) override;
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_PROVIDER_MAC_H_
