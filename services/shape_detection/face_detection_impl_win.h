// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_

#include <windows.foundation.h>
#include <wrl/client.h>

#include "detection_utils_win.h"
#include "services/shape_detection/public/interfaces/facedetection.mojom.h"

class SkBitmap;

namespace ABI {
namespace Windows {
namespace Media {
namespace FaceAnalysis {
struct IFaceDetectorStatics;
struct FaceDetector;
struct IFaceDetector;
}  // namespace FaceAnalysis
}  // namespace Media
}  // namespace Windows
}  // namespace ABI

enum AsyncOperationStatus {
  ASYNC_OPERATING = 1,
  ASYNC_FAILED,
  ASYNC_IDLE,
};

namespace shape_detection {

using ABI::Windows::Foundation::IAsyncOperation;

class FaceDetectionImplWin : public mojom::FaceDetection {
 public:
  using IFaceDetectorStatics =
      ABI::Windows::Media::FaceAnalysis::IFaceDetectorStatics;
  using FaceDetector = ABI::Windows::Media::FaceAnalysis::FaceDetector;
  using IFaceDetector = ABI::Windows::Media::FaceAnalysis::IFaceDetector;

  static std::unique_ptr<FaceDetectionImplWin> Create();

  explicit FaceDetectionImplWin(
      Microsoft::WRL::ComPtr<IFaceDetectorStatics> factory);
  ~FaceDetectionImplWin() override;

  void Detect(const SkBitmap& bitmap,
              mojom::FaceDetection::DetectCallback callback) override;

 private:
  Microsoft::WRL::ComPtr<IFaceDetectorStatics> face_detector_factory_;

  HRESULT CreateFaceDetector();
  void OnFaceDetectorCreated(
      Microsoft::WRL::ComPtr<IAsyncOperation<FaceDetector*>> async_op);
  std::unique_ptr<AsyncOperation<FaceDetector>> async_create_detector_ops_;

  Microsoft::WRL::ComPtr<IFaceDetector> face_detector_;

  FRIEND_TEST_ALL_PREFIXES(FaceDetectionImplWinTest, CreateFaceDetectorFailed);
  friend class FaceDetectionImplWinTest;
  void SetAsyncStatus(AsyncOperationStatus status) { async_status_ = status; }
  AsyncOperationStatus AsyncStatus() { return async_status_; }
  AsyncOperationStatus async_status_;

  DISALLOW_COPY_AND_ASSIGN(FaceDetectionImplWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_