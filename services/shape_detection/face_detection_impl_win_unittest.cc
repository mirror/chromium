// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include "base/test/scoped_task_environment.h"
#include "base/win/scoped_com_initializer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace shape_detection {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

class FaceDetectionImplWinTest : public testing::Test {
 protected:
  void SetUp() override {
    scoped_com_initializer_ = base::MakeUnique<base::win::ScopedCOMInitializer>(
        base::win::ScopedCOMInitializer::kMTA);
    ASSERT_TRUE(scoped_com_initializer_->Succeeded());
  }

 public:
  void CreateDetectorCallback() {
    // Create Face detector succeeded.
    EXPECT_EQ(ASYNC_IDLE, impl_->AsyncStatus());
  }

  void DetectFailedCallback(
      std::vector<mojom::FaceDetectionResultPtr> results) {
    ASSERT_EQ(0u, results.size());
    Detection();
  }

  MOCK_METHOD0(Detection, void(void));

  std::unique_ptr<FaceDetectionImplWin> impl_;

 private:
  std::unique_ptr<base::win::ScopedCOMInitializer> scoped_com_initializer_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(FaceDetectionImplWinTest, CreateAndDestroy) {
  auto impl = FaceDetectionImplWin::Create();
}

TEST_F(FaceDetectionImplWinTest, CreateFaceDetectorSucceeded) {
  impl_ = FaceDetectionImplWin::Create();
  if (!impl_)
    return;

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&FaceDetectionImplWinTest::CreateDetectorCallback,
                     base::Unretained(this)),
      base::TimeDelta::FromSeconds(1));

  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::TimeDelta::FromSeconds(1));
  run_loop.Run();
}

TEST_F(FaceDetectionImplWinTest, CreateFaceDetectorFailed) {
  auto impl = FaceDetectionImplWin::Create();
  if (!impl)
    return;

  // Set failed status when creating face detector.
  SkBitmap bitmap;
  bitmap.setInfo(SkImageInfo::MakeN32Premul(100, 100));
  impl->SetAsyncStatus(ASYNC_FAILED);

  base::RunLoop run_loop;
  EXPECT_CALL(*this, Detection()).WillOnce(RunClosure(run_loop.QuitClosure()));
  impl->Detect(bitmap,
               base::BindOnce(&FaceDetectionImplWinTest::DetectFailedCallback,
                              base::Unretained(this)));
  run_loop.Run();
}

}  // namespace shape_detection
