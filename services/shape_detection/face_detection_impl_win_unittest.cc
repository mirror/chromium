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
 protected:
  void SetUp() override {
    scoped_com_initializer_ = base::MakeUnique<base::win::ScopedCOMInitializer>(
        base::win::ScopedCOMInitializer::kMTA);
    ASSERT_TRUE(scoped_com_initializer_->succeeded());
  }

  void TearDown() override { scoped_com_initializer_.reset(); }

 private:
  std::unique_ptr<base::win::ScopedCOMInitializer> scoped_com_initializer_;
};

TEST_F(FaceDetectionImplWinTest, CreateAndDestroy) {
  auto impl = base::MakeUnique<FaceDetectionImplWin>();
}

TEST_F(FaceDetectionImplWinTest, ScanOneFace) {
  auto factory = FaceDetectionImplWin::GetFactory();
  if (!factory)
    return;

  auto impl = base::MakeUnique<FaceDetectionImplWin>();
  impl->SetFaceDetectorFactory(std::move(factory));
}

}  // namespace shape_detection
