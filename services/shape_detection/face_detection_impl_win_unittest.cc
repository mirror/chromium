// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_win.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/win/scoped_com_initializer.h"
#include "face_detection_impl_win.h"
#include "face_detection_provider_win.h"
#include "services/shape_detection/public/interfaces/facedetection_provider.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/codec/jpeg_codec.h"

namespace shape_detection {

class FaceDetectionImplWinTest : public testing::Test {
 protected:
  void SetUp() override {
    scoped_com_initializer_ = std::make_unique<base::win::ScopedCOMInitializer>(
        base::win::ScopedCOMInitializer::kMTA);
    ASSERT_TRUE(scoped_com_initializer_->Succeeded());
  }

 public:
  void CreateFaceDetectorCallback(bool succeeded) { EXPECT_TRUE(succeeded); }

  void DetectCallback(base::Closure quit_closure,
                      uint32_t* faces,
                      std::vector<mojom::FaceDetectionResultPtr> results) {
    *faces = results.size();

    CloseConnection(quit_closure);
  }

  void CloseConnection(base::Closure quit_closure) { quit_closure.Run(); }

 private:
  std::unique_ptr<base::win::ScopedCOMInitializer> scoped_com_initializer_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(FaceDetectionImplWinTest, CreateFaceDetector) {
  mojom::FaceDetectionProviderPtr provider;
  mojom::FaceDetectionPtr face_service;

  auto request = mojo::MakeRequest(&provider);
  auto provider_win = base::MakeUnique<FaceDetectionProviderWin>();
  auto* provider_ptr = provider_win.get();
  // A raw pointer is obtained because ownership is passed to MakeStrongBinding.
  provider_ptr->binding_ =
      mojo::MakeStrongBinding(std::move(provider_win), std::move(request));

  auto options = shape_detection::mojom::FaceDetectorOptions::New();
  provider->CreateFaceDetection(mojo::MakeRequest(&face_service),
                                std::move(options));

  SkBitmap bitmap;
  bitmap.allocN32Pixels(100, 100);
  bitmap.eraseColor(SK_ColorBLUE);

  base::RunLoop run_loop;
  g_callback_for_testing =
      base::BindOnce(&FaceDetectionImplWinTest::CreateFaceDetectorCallback,
                     base::Unretained(this));

  face_service.set_connection_error_handler(
      base::Bind(&FaceDetectionImplWinTest::CloseConnection,
                 base::Unretained(this), run_loop.QuitClosure()));
  uint32_t faces;
  face_service->Detect(
      bitmap,
      base::BindOnce(&FaceDetectionImplWinTest::DetectCallback,
                     base::Unretained(this), run_loop.QuitClosure(), &faces));
  run_loop.Run();
  EXPECT_EQ(0u, faces);
}

TEST_F(FaceDetectionImplWinTest, ScanOneFace) {
  mojom::FaceDetectionProviderPtr provider;
  mojom::FaceDetectionPtr face_service;

  auto request = mojo::MakeRequest(&provider);
  auto provider_win = base::MakeUnique<FaceDetectionProviderWin>();
  auto* provider_ptr = provider_win.get();
  // A raw pointer is obtained because ownership is passed to MakeStrongBinding.
  provider_ptr->binding_ =
      mojo::MakeStrongBinding(std::move(provider_win), std::move(request));

  auto options = shape_detection::mojom::FaceDetectorOptions::New();
  provider->CreateFaceDetection(mojo::MakeRequest(&face_service),
                                std::move(options));

  // Load image data from test directory.
  base::FilePath image_path;
  ASSERT_TRUE(PathService::Get(base::DIR_SOURCE_ROOT, &image_path));
  image_path = image_path.Append(FILE_PATH_LITERAL("services"))
                   .Append(FILE_PATH_LITERAL("test"))
                   .Append(FILE_PATH_LITERAL("data"))
                   .Append(FILE_PATH_LITERAL("mona_lisa.jpg"));
  ASSERT_TRUE(base::PathExists(image_path));
  std::string image_data;
  ASSERT_TRUE(base::ReadFileToString(image_path, &image_data));

  std::unique_ptr<SkBitmap> image = gfx::JPEGCodec::Decode(
      reinterpret_cast<const uint8_t*>(image_data.data()), image_data.size());
  ASSERT_TRUE(image);

  const gfx::Size size(image->width(), image->height());
  const uint32_t num_bytes = size.GetArea() * 4 /* bytes per pixel */;
  ASSERT_EQ(num_bytes, image->computeByteSize());

  base::RunLoop run_loop;
  face_service.set_connection_error_handler(
      base::Bind(&FaceDetectionImplWinTest::CloseConnection,
                 base::Unretained(this), run_loop.QuitClosure()));
  uint32_t faces;
  face_service->Detect(
      *image,
      base::BindOnce(&FaceDetectionImplWinTest::DetectCallback,
                     base::Unretained(this), run_loop.QuitClosure(), &faces));
  run_loop.Run();
  EXPECT_EQ(1u, faces);
}

}  // namespace shape_detection
