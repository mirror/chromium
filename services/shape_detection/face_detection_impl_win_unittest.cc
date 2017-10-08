// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include <wrl\wrappers\corewrappers.h>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/platform_thread.h"
#include "base/win/scoped_com_initializer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gl/gl_switches.h"

using ::testing::Invoke;
using ::testing::TestWithParam;
using ::testing::ValuesIn;

namespace shape_detection {

namespace {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

const int kJpegImageWidth = 120;
const int kJpegImageHeight = 120;
const char kJpegImagePath[] = "services/test/data/mona_lisa.jpg";

}  // anonymous namespace

class FaceDetectionImplWinTest : public TestWithParam<bool> {
 public:
  FaceDetectionImplWinTest()
      : scoped_com_initializer_(base::win::ScopedCOMInitializer::kMTA),
        winrt_initializer_(RO_INIT_MULTITHREADED) {
    DLOG_IF(ERROR, FAILED(winrt_initializer_)) << "RoInitializeWrapper()";
  }

  ~FaceDetectionImplWinTest() override {}

  void DetectCallback(std::vector<mojom::FaceDetectionResultPtr> results) {
    ASSERT_EQ(1u, results.size());
    Detection();
  }
  MOCK_METHOD0(Detection, void(void));

  base::win::ScopedCOMInitializer scoped_com_initializer_;
  Microsoft::WRL::Wrappers::RoInitializeWrapper winrt_initializer_;

  std::unique_ptr<FaceDetectionImplWin> impl_;
  const base::MessageLoop message_loop_;
};

TEST_P(FaceDetectionImplWinTest, ScanOneFace) {
  auto options = shape_detection::mojom::FaceDetectorOptions::New();
  options->fast_mode = GetParam();
  impl_ = base::MakeUnique<FaceDetectionImplWin>(std::move(options));

  base::RunLoop().RunUntilIdle();

  // Load image data from test directory.
  base::FilePath image_path;
  ASSERT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &image_path));
  image_path = image_path.AppendASCII(kJpegImagePath);
  ASSERT_TRUE(base::PathExists(image_path));
  std::string image_data;
  ASSERT_TRUE(base::ReadFileToString(image_path, &image_data));

  std::unique_ptr<SkBitmap> image = gfx::JPEGCodec::Decode(
      reinterpret_cast<const uint8_t*>(image_data.data()), image_data.size());
  ASSERT_TRUE(image);
  ASSERT_EQ(kJpegImageWidth, image->width());
  ASSERT_EQ(kJpegImageHeight, image->height());

  const gfx::Size size(image->width(), image->height());
  const int num_bytes = size.GetArea() * 4 /* bytes per pixel */;
  ASSERT_EQ(num_bytes, image->computeSize64());

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();
  // Send the image to Detect() and expect the response in callback.
  EXPECT_CALL(*this, Detection()).WillOnce(RunClosure(quit_closure));
  impl_->Detect(*image, base::Bind(&FaceDetectionImplWinTest::DetectCallback,
                                   base::Unretained(this)));
  run_loop.Run();
}

INSTANTIATE_TEST_CASE_P(, FaceDetectionImplWinTest, ValuesIn({true, false}));

}  // namespace shape_detection
