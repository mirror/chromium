// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include <windows.media.faceanalysis.h>
#include <wrl\wrappers\corewrappers.h>

#include "base/win/scoped_com_initializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shape_detection {

class FaceDetectionImplWinTest : public testing::Test {
 public:
  FaceDetectionImplWinTest() : winrt_initializer_(RO_INIT_MULTITHREADED) {
    DLOG_IF(ERROR, FAILED(winrt_initializer_))
        << "RoInitializeWrapper()"
        << logging::SystemErrorCodeToString(winrt_initializer_);
  }

 protected:
  Microsoft::WRL::Wrappers::RoInitializeWrapper winrt_initializer_;
};

TEST_F(FaceDetectionImplWinTest, CreateAndDestroy) {
  auto impl = base::MakeUnique<FaceDetectionImplWin>();
}

TEST_F(FaceDetectionImplWinTest, ScanOneFace) {
  auto factory = FaceDetectionImplWin::GetFactory();
  if (!factory)
    return;

  auto impl = base::MakeUnique<FaceDetectionImplWin>();
  impl->SetFaceDetectorFactory(factory);
}

}  // namespace shape_detection
