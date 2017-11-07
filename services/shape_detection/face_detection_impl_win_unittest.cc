// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include "base/test/scoped_task_environment.h"
#include "base/win/scoped_com_initializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shape_detection {

class FaceDetectionImplWinTest : public testing::Test {
 protected:
  void SetUp() override {
    scoped_com_initializer_ = base::MakeUnique<base::win::ScopedCOMInitializer>(
        base::win::ScopedCOMInitializer::kMTA);
    ASSERT_TRUE(scoped_com_initializer_->Succeeded());
  }

 public:
  void CreateDetectorSucceeded(base::Closure quite_closure) {
    // Create Face detector succeeded.
    quite_closure.Run();
  }

  void DetectFailedCallback(
      base::Closure quite_closure,
      std::vector<mojom::FaceDetectionResultPtr> results) {
    EXPECT_EQ(0u, results.size());

    quite_closure.Run();
  }

 private:
  std::unique_ptr<base::win::ScopedCOMInitializer> scoped_com_initializer_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(FaceDetectionImplWinTest, CreateAndDestroy) {
  auto impl = FaceDetectionImplWin::Create();
}

TEST_F(FaceDetectionImplWinTest, CreateFaceDetectorSucceeded) {
  auto impl = FaceDetectionImplWin::Create();
  if (!impl)
    return;

  base::RunLoop run_loop;
  impl->callback_for_testing_ =
      base::BindOnce(&FaceDetectionImplWinTest::CreateDetectorSucceeded,
                     base::Unretained(this), run_loop.QuitClosure());
  run_loop.Run();
}

TEST_F(FaceDetectionImplWinTest, CreateFaceDetectorFailed) {
  auto impl = FaceDetectionImplWin::Create();
  if (!impl)
    return;

  // Set failed status when creating face detector.
  SkBitmap bitmap;
  bitmap.setInfo(SkImageInfo::MakeN32Premul(100, 100));
  impl->async_status_ = AsyncOperationStatus::KFailed;

  base::RunLoop run_loop;
  impl->Detect(bitmap,
               base::BindOnce(&FaceDetectionImplWinTest::DetectFailedCallback,
                              base::Unretained(this), run_loop.QuitClosure()));
  run_loop.Run();
}

}  // namespace shape_detection
