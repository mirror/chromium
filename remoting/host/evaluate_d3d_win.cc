// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/evaluate_d3d_win.h"

#include <D3DCommon.h>

#include <iostream>

#include "remoting/host/host_exit_codes.h"
#include "third_party/webrtc/modules/desktop_capture/win/dxgi_duplicator_controller.h"
#include "third_party/webrtc/modules/desktop_capture/win/screen_capturer_win_directx.h"

namespace remoting {

int EvaluateD3D() {
  // Creates a capturer instance to avoid the DxgiDuplicatorController to be
  // initialized and deinitialized for several times.
  webrtc::ScreenCapturerWinDirectx capturer;

  if (webrtc::ScreenCapturerWinDirectx::IsSupported()) {
    // Guaranteed to work.
    std::cout << "DirectX-Capturer" << std::endl;
  } else if (webrtc::ScreenCapturerWinDirectx::IsCurrentSessionSupported()) {
    // If we are in a supported session, but DirectX capturer is not able to be
    // initialized. Something must be wrong, we should actively disable it.
    std::cout << "No-DirectX-Capturer" << std::endl;
  }

  webrtc::DxgiDuplicatorController::D3dInfo info;
  webrtc::ScreenCapturerWinDirectx::RetrieveD3dInfo(&info);
  if (info.min_feature_level < D3D_FEATURE_LEVEL_10_0) {
    std::cout << "MinD3DLT10" << std::endl;
  } else {
    std::cout << "MinD3DGT10" << std::endl;
  }
  if (info.min_feature_level >= D3D_FEATURE_LEVEL_11_0) {
    std::cout << "MinD3DGT11" << std::endl;
  }
  if (info.min_feature_level >= D3D_FEATURE_LEVEL_12_0) {
    std::cout << "MinD3DGT12" << std::endl;
  }

  return kSuccessExitCode;
}

}  // namespace remoting
