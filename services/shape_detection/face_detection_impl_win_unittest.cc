// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include <windows.media.faceanalysis.h>
#include <wrl\wrappers\corewrappers.h>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/platform_thread.h"
#include "base/win/scoped_com_initializer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gl/gl_switches.h"

using ::testing::TestWithParam;
using ::testing::ValuesIn;

namespace shape_detection {

class FaceDetectionImplWinTest : public TestWithParam<bool> {
 public:
  FaceDetectionImplWinTest()
      : scoped_com_initializer_(base::win::ScopedCOMInitializer::kMTA),
        winrt_initializer_(RO_INIT_MULTITHREADED) {
    DLOG_IF(ERROR, FAILED(winrt_initializer_)) << "RoInitializeWrapper()";
  }

  ~FaceDetectionImplWinTest() override {}

  base::win::ScopedCOMInitializer scoped_com_initializer_;
  Microsoft::WRL::Wrappers::RoInitializeWrapper winrt_initializer_;

  std::unique_ptr<FaceDetectionImplWin> impl_;
  const base::MessageLoop message_loop_;
};

TEST_F(FaceDetectionImplWinTest, CreateAndDestroy) {
  impl_ = base::MakeUnique<FaceDetectionImplWin>(
      shape_detection::mojom::FaceDetectorOptions::New());
}

TEST_P(FaceDetectionImplWinTest, ScanOneFace) {
  // Face detection test needs a GPU.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseGpuInTests))
    return;

  auto factory = FaceDetectionImplWin::GetFactory();
  if (!factory)
    return;

  auto options = shape_detection::mojom::FaceDetectorOptions::New();
  options->fast_mode = GetParam();
  impl_ = base::MakeUnique<FaceDetectionImplWin>(std::move(options));
  impl_->SetFaceDetectorFactory(factory);
}

INSTANTIATE_TEST_CASE_P(, FaceDetectionImplWinTest, ValuesIn({true, false}));

}  // namespace shape_detection
