// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_

#include <windows.foundation.h>
#include <windows.media.faceanalysis.h>

#include <unordered_set>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/win/scoped_comptr.h"
#include "services/shape_detection/detection_utils_win.h"
#include "services/shape_detection/public/interfaces/facedetection.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace shape_detection {

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Graphics::Imaging;
using namespace ABI::Windows::Media::FaceAnalysis;

class FaceDetectionImplWin : public shape_detection::mojom::FaceDetection {
 public:
  explicit FaceDetectionImplWin(
      shape_detection::mojom::FaceDetectorOptionsPtr options);
  ~FaceDetectionImplWin() override;

  void Detect(
      const SkBitmap& bitmap,
      shape_detection::mojom::FaceDetection::DetectCallback callback) override;

 private:
  void asyncDetectSoftwareBitmap();
  void asyncCreateFaceDetector();

  class AsyncCreateFaceDetector;
  class AsyncDetectFace;

  void OnFaceDetectorConstructed(IAsyncOperation<FaceDetector*>* async_op);
  void OnFaceDetected(IAsyncOperation<IVector<DetectedFace*>*>* async_op);

  base::win::ScopedComPtr<IFaceDetectorStatics> detector_factory_;
  base::win::ScopedComPtr<IFaceDetector> detector_;
  std::unique_ptr<AsyncCreateFaceDetector> async_create_detector_ops_;

  base::win::ScopedComPtr<IVector<DetectedFace*>> detected_face_;
  std::unique_ptr<AsyncDetectFace> async_detected_face_ops_;

  base::win::ScopedComPtr<ISoftwareBitmap> convertToSoftwareBitmap(
      const SkBitmap& bitmap);

  // TODO(Junwei): Use vector to hold |detect_callback_| and |bitmap_|
  DetectCallback detect_callback_;
  SkBitmap bitmap_;

  base::win::ScopedComPtr<ISoftwareBitmapStatics> bitmap_factory_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(FaceDetectionImplWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_