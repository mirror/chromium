// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_
#define SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_

#include <windows.foundation.h>
#include <windows.media.faceanalysis.h>
#pragma warning(disable : 4002)
#include <windows.ui.xaml.media.imaging.h>
#pragma warning(default : 4002)

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
using namespace ABI::Windows::UI::Xaml::Media::Imaging;

class FaceDetectionImplWin : public shape_detection::mojom::FaceDetection {
 public:
  explicit FaceDetectionImplWin(
      shape_detection::mojom::FaceDetectorOptionsPtr options);
  ~FaceDetectionImplWin() override;

  void Detect(
      const SkBitmap& bitmap,
      shape_detection::mojom::FaceDetection::DetectCallback callback) override;

 private:
  class AsyncCreateFaceDetector
      : public AsyncOperationTemplate<FaceDetector, FaceDetectionImplWin> {
   public:
    AsyncCreateFaceDetector(FaceDetectionImplWin* face_detection_impl)
        : AsyncOperationTemplate(face_detection_impl), weak_factory_(this) {}

   private:
    void asyncCallback(IAsyncOperation<FaceDetector*>* async_op) {
      detection_impl_->OnFaceDetectorConstructed(async_op);
    }
    base::WeakPtr<AsyncOperationTemplate> GetWeakPtrFromFactory() final {
      return weak_factory_.GetWeakPtr();
    }
    base::WeakPtrFactory<AsyncCreateFaceDetector> weak_factory_;
  };
  void OnFaceDetectorConstructed(IAsyncOperation<FaceDetector*>* async_op);

  void asyncDetectSoftwareBitmap();
  void asyncCreateFaceDetector();

  base::win::ScopedComPtr<ISoftwareBitmap> convertToSoftwareBitmap(
      const SkBitmap& bitmap);

  base::win::ScopedComPtr<IFaceDetectorStatics> detector_factory_;
  base::win::ScopedComPtr<IFaceDetector> detector_;
  AsyncCreateFaceDetector async_create_detector_ops_;

  class AsyncDetectFace : public AsyncOperationTemplate<IVector<DetectedFace*>,
                                                        FaceDetectionImplWin> {
   public:
    AsyncDetectFace(FaceDetectionImplWin* face_detection_impl)
        : AsyncOperationTemplate(face_detection_impl), weak_factory_(this) {}

   private:
    void asyncCallback(IAsyncOperation<IVector<DetectedFace*>*>* async_op) {
      detection_impl_->OnFacesDetected(async_op);
    }
    base::WeakPtr<AsyncOperationTemplate> GetWeakPtrFromFactory() final {
      return weak_factory_.GetWeakPtr();
    }
    base::WeakPtrFactory<AsyncDetectFace> weak_factory_;
  };
  base::win::ScopedComPtr<IVector<DetectedFace*>> detected_face_;
  AsyncDetectFace async_detected_face_ops_;
  void OnFacesDetected(IAsyncOperation<IVector<DetectedFace*>*>* async_op);
  // AsyncOperationTemplate<DetectedFace, FaceDetectionImplWin>
  // detect_face_ops_;

  // TODO(Junwei): Use vector to hold |detect_callback_| and |bitmap_|
  DetectCallback detect_callback_;
  SkBitmap bitmap_;
  bool detecting_bitmap_;

  base::win::ScopedComPtr<ISoftwareBitmapStatics> bitmap_factory_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(FaceDetectionImplWin);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_FACE_DETECTION_IMPL_WIN_H_