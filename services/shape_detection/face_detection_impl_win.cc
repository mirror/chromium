// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include <windows.media.faceanalysis.h>

namespace shape_detection {

base::OnceCallback<void(bool)> g_callback_for_testing;

FaceDetectionImplWin::FaceDetectionImplWin(
    Microsoft::WRL::ComPtr<IFaceDetector> face_detector,
    Microsoft::WRL::ComPtr<ISoftwareBitmapStatics> bitmap_factory,
    BitmapPixelFormat pixel_format)
    : face_detector_(std::move(face_detector)),
      bitmap_factory_(std::move(bitmap_factory)),
      pixel_format_(pixel_format) {}
FaceDetectionImplWin::~FaceDetectionImplWin() = default;

void FaceDetectionImplWin::Detect(const SkBitmap& bitmap,
                                  DetectCallback callback) {
  // |face_detector_| has created before binding FaceDetectionImplWin to
  // FaceDetectionRequest.
  if (g_callback_for_testing) {
    std::move(g_callback_for_testing).Run(face_detector_ ? true : false);
    std::move(callback).Run({});
  }

  Microsoft::WRL::ComPtr<ISoftwareBitmap> win_bitmap =
      CreateWinBitmapFromSkBitmap(bitmap_factory_.Get(), pixel_format_, bitmap);
  if (!win_bitmap) {
    std::move(callback).Run({});
    return;
  }

  // Detect faces asynchronously.
  AsyncOperation<IVector<DetectedFace*>>::IAsyncOperationPtr async_op;
  const HRESULT hr =
      face_detector_->DetectFacesAsync(win_bitmap.Get(), &async_op);
  if (FAILED(hr)) {
    std::move(callback).Run({});
    DLOG(ERROR) << "Detect faces asynchronously failed: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  // The once callback will not be called if this object is deleted, so it's
  // fine to use Unretained to bind the callback.
  auto async_operation = AsyncOperation<IVector<DetectedFace*>>::Create(
      base::BindOnce(&FaceDetectionImplWin::OnFaceDetected,
                     base::Unretained(this)),
      std::move(async_op));
  if (!async_operation) {
    std::move(callback).Run({});
    return;
  }

  async_detect_face_ops_ = std::move(async_operation);
  detected_face_callback_ = std::move(callback);
  // This prevents the Detect function from being called before the
  // AsyncOperation completes.
  binding_->PauseIncomingMethodCallProcessing();
}

void FaceDetectionImplWin::OnFaceDetected(
    AsyncOperation<IVector<DetectedFace*>>::IAsyncOperationPtr async_op) {
  binding_->ResumeIncomingMethodCallProcessing();
  async_detect_face_ops_.reset();

  Microsoft::WRL::ComPtr<IVector<DetectedFace*>> detected_face;
  HRESULT hr =
      async_op ? async_op->GetResults(detected_face.GetAddressOf()) : E_FAIL;
  if (FAILED(hr)) {
    std::move(detected_face_callback_).Run({});
    DLOG(ERROR) << "GetResults failed: "
                << logging::SystemErrorCodeToString(hr);
    return;
  }

  std::vector<mojom::FaceDetectionResultPtr> results;
  uint32_t count;
  hr = detected_face->get_Size(&count);
  if (FAILED(hr)) {
    std::move(detected_face_callback_).Run({});
    DLOG(ERROR) << "get_Size failed: " << logging::SystemErrorCodeToString(hr);
    return;
  }
  for (uint32_t i = 0; i < count; i++) {
    Microsoft::WRL::ComPtr<IDetectedFace> face;
    hr = detected_face->GetAt(i, &face);
    if (FAILED(hr))
      break;

    ABI::Windows::Graphics::Imaging::BitmapBounds bounds;
    hr = face->get_FaceBox(&bounds);
    if (FAILED(hr))
      break;

    gfx::RectF boundingbox(bounds.X, bounds.Y, bounds.Width, bounds.Height);
    auto result = shape_detection::mojom::FaceDetectionResult::New();
    result->bounding_box = std::move(boundingbox);
    results.push_back(std::move(result));
  }

  std::move(detected_face_callback_).Run(std::move(results));
}

}  // namespace shape_detection