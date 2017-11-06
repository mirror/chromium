// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include <windows.media.faceanalysis.h>

#include "base/scoped_generic.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "base/win/windows_version.h"
#include "media/base/scoped_callback_runner.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/shape_detection/face_detection_provider_impl.h"

namespace shape_detection {

using ABI::Windows::Foundation::IAsyncOperation;
using ABI::Windows::Media::FaceAnalysis::FaceDetector;
using base::win::ScopedHString;
using base::win::GetActivationFactory;

void FaceDetectionProviderImpl::CreateFaceDetection(
    shape_detection::mojom::FaceDetectionRequest request,
    shape_detection::mojom::FaceDetectorOptionsPtr options) {
  auto impl = FaceDetectionImplWin::Create();
  if (!impl)
    return;

  mojo::MakeStrongBinding(std::move(impl), std::move(request));
}

// static
std::unique_ptr<FaceDetectionImplWin> FaceDetectionImplWin::Create() {
  // FaceDetector class is only available in Win 10 onwards (v10.0.10240.0).
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    DVLOG(1) << "FaceDetector not supported before Windows 10";
    return nullptr;
  }
  // Loads functions dynamically at runtime to prevent library dependencies.
  if (!(base::win::ResolveCoreWinRTDelayload() &&
        ScopedHString::ResolveCoreWinRTStringDelayload())) {
    DLOG(ERROR) << "Failed loading functions from combase.dll";
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IFaceDetectorStatics> factory;
  HRESULT hr = GetActivationFactory<
      IFaceDetectorStatics,
      RuntimeClass_Windows_Media_FaceAnalysis_FaceDetector>(&factory);
  if (FAILED(hr)) {
    DLOG(ERROR) << "IFaceDetectorStatics factory failed: "
                << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }

  boolean is_supported = FALSE;
  factory->get_IsSupported(&is_supported);
  if (is_supported == FALSE) {
    DLOG(ERROR) << "FaceDetector isn't supported on the current device.";
    return nullptr;
  }

  // Create a instance of FaceDetector asynchronously.
  auto impl = std::make_unique<FaceDetectionImplWin>(std::move(factory));
  hr = impl->CreateFaceDetector();
  if (FAILED(hr)) {
    DLOG(ERROR) << "Async Create FaceDetector failed: "
                << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }

  return impl;
}

FaceDetectionImplWin::FaceDetectionImplWin(
    Microsoft::WRL::ComPtr<IFaceDetectorStatics> factory)
    : face_detector_factory_(std::move(factory)) {}

FaceDetectionImplWin::~FaceDetectionImplWin() = default;

void FaceDetectionImplWin::Detect(const SkBitmap& bitmap,
                                  DetectCallback callback) {
  DCHECK_EQ(kN32_SkColorType, bitmap.colorType());

  if (async_status_ == AsyncOperationStatus::ASYNC_OPERATING) {
    // TODO(junwei.fu): Save |callback| and |bitmap| for detecting seriality.
  } else if (async_status_ == AsyncOperationStatus::ASYNC_IDLE) {
    // TODO(junwei.fu): Detect face async.
  } else if (async_status_ == AsyncOperationStatus::ASYNC_FAILED) {
    // Return face 0.
    std::move(callback).Run(std::vector<mojom::FaceDetectionResultPtr>());
  }
}

// Create a instance of FaceDetector asynchronously.
HRESULT FaceDetectionImplWin::CreateFaceDetector() {
  Microsoft::WRL::ComPtr<IAsyncOperation<FaceDetector*>> async_op;
  HRESULT hr = face_detector_factory_->CreateAsync(&async_op);
  if (FAILED(hr)) {
    DLOG(ERROR) << "Create FaceDetector failed: "
                << logging::SystemErrorCodeToString(hr);
    return hr;
  }

  // The once callback will not be called if this object is deleted,
  // so we can use Unretained to bind the callback.
  auto async_operation = AsyncOperation<FaceDetector>::Create(
      base::BindOnce(&FaceDetectionImplWin::OnFaceDetectorCreated,
                     base::Unretained(this)),
      std::move(async_op));
  if (!async_operation)
    return E_FAIL;

  async_create_detector_ops_ = std::move(async_operation);
  async_status_ = AsyncOperationStatus::ASYNC_OPERATING;

  return S_OK;
}

void FaceDetectionImplWin::OnFaceDetectorCreated(
    Microsoft::WRL::ComPtr<IAsyncOperation<FaceDetector*>> async_op) {
  const HRESULT hr =
      async_op ? async_op->GetResults(face_detector_.GetAddressOf()) : E_FAIL;
  if (FAILED(hr)) {
    DLOG(ERROR) << "GetResults failed: "
                << logging::SystemErrorCodeToString(hr);
    async_status_ = AsyncOperationStatus::ASYNC_FAILED;
    return;
  }

  async_create_detector_ops_.reset();

  // The callback only for unittests if async Face Detector is succeeded.
  if (!callback_for_testing_.is_null())
    std::move(callback_for_testing_).Run();
}

}  // namespace shape_detection