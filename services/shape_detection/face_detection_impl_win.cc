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

using namespace ABI::Windows::Graphics::Imaging;
using namespace ABI::Windows::Media::FaceAnalysis;

using base::win::ScopedHString;
using base::win::GetActivationFactory;

// This is a static method that is used by the unit tests and
// so needs to be available outside this compilation unit.
Microsoft::WRL::ComPtr<IFaceDetectorStatics>
FaceDetectionImplWin::GetFactory() {
  // FaceDetector class is only available in Win 10 onwards (v10.0.10240.0).
  if (base::win::GetVersion() < base::win::VERSION_WIN10) {
    DVLOG(1) << "FaceDetector not supported before Windows 10";
    return nullptr;
  }
  // Loads functions dynamically at runtime to prevent library dependencies.
  if (!(base::win::ResolveCoreWinRTDelayload() &&
        ScopedHString::ResolveCoreWinRTStringDelayload())) {
    DPLOG(ERROR) << "Failed loading functions from combase.dll";
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IFaceDetectorStatics> factory;
  HRESULT hr = GetActivationFactory<
      IFaceDetectorStatics,
      RuntimeClass_Windows_Media_FaceAnalysis_FaceDetector>(&factory);
  if (FAILED(hr)) {
    DPLOG(ERROR) << "IFaceDetectorStatics factory failed";
    return nullptr;
  }

  boolean is_supported = FALSE;
  factory->get_IsSupported(&is_supported);
  return (is_supported == FALSE) ? nullptr : factory;
}

void FaceDetectionProviderImpl::CreateFaceDetection(
    shape_detection::mojom::FaceDetectionRequest request,
    shape_detection::mojom::FaceDetectorOptionsPtr options) {
  auto factory = FaceDetectionImplWin::GetFactory();
  if (!factory)
    return;

  auto impl = base::MakeUnique<FaceDetectionImplWin>(std::move(options));
  impl->SetFaceDetectorFactory(factory);

  mojo::MakeStrongBinding(std::move(impl), std::move(request));
}

FaceDetectionImplWin::FaceDetectionImplWin(
    shape_detection::mojom::FaceDetectorOptionsPtr options) {}

FaceDetectionImplWin::~FaceDetectionImplWin() {}

void FaceDetectionImplWin::Detect(const SkBitmap& bitmap,
                                  DetectCallback callback) {
  DCHECK_EQ(bitmap.colorType(), kN32_SkColorType);
}

}  // namespace shape_detection