// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_

#include <windows.foundation.h>
#include <windows.graphics.imaging.h>
#include <wrl/client.h>

#include "detection_utils_win.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/public/interfaces/facedetection.mojom.h"

class SkBitmap;

namespace ABI {
namespace Windows {
namespace Media {
namespace FaceAnalysis {
interface IFaceDetector;
class FaceDetector;
class DetectedFace;
}  // namespace FaceAnalysis
}  // namespace Media
}  // namespace Windows
}  // namespace ABI

using ABI::Windows::Foundation::Collections::IVector;

namespace shape_detection {

extern base::OnceCallback<void(bool)> g_callback_for_testing;

class FaceDetectionImplWin : public mojom::FaceDetection {
 public:
  using FaceDetector = ABI::Windows::Media::FaceAnalysis::FaceDetector;
  using IFaceDetector = ABI::Windows::Media::FaceAnalysis::IFaceDetector;
  using BitmapPixelFormat = ABI::Windows::Graphics::Imaging::BitmapPixelFormat;
  using ISoftwareBitmapStatics =
      ABI::Windows::Graphics::Imaging::ISoftwareBitmapStatics;
  using DetectedFace = ABI::Windows::Media::FaceAnalysis::DetectedFace;
  using IDetectedFace = ABI::Windows::Media::FaceAnalysis::IDetectedFace;

  FaceDetectionImplWin(
      Microsoft::WRL::ComPtr<IFaceDetector> face_detector,
      Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory,
      BitmapPixelFormat pixel_format);
  ~FaceDetectionImplWin() override;

  void SetBinding(mojo::StrongBindingPtr<mojom::FaceDetection> binding) {
    binding_ = std::move(binding);
  }

  // mojom::FaceDetection implementation.
  void Detect(const SkBitmap& bitmap,
              mojom::FaceDetection::DetectCallback callback) override;

 private:
  void OnFaceDetected(
      AsyncOperation<IVector<DetectedFace*>>::IAsyncOperationPtr async_op);

  Microsoft::WRL::ComPtr<IFaceDetector> face_detector_;

  Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory_;
  BitmapPixelFormat pixel_format_;

  std::unique_ptr<AsyncOperation<IVector<DetectedFace*>>>
      async_detect_face_ops_;
  DetectCallback detected_face_callback_;
  mojo::StrongBindingPtr<mojom::FaceDetection> binding_;

  DISALLOW_COPY_AND_ASSIGN(FaceDetectionImplWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_