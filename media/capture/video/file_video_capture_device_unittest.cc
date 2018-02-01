// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "media/base/bind_to_current_loop.h"
#include "media/base/test_data_util.h"
#include "media/capture/video/file_video_capture_device.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace media {

namespace {

class MockClient : public VideoCaptureDevice::Client {
 public:
  MOCK_METHOD7(OnIncomingCapturedData,
               void(const uint8_t*,
                    int,
                    const VideoCaptureFormat&,
                    int,
                    base::TimeTicks,
                    base::TimeDelta,
                    int));

  MOCK_METHOD4(
      ReserveOutputBuffer,
      Buffer(const gfx::Size&, VideoPixelFormat, VideoPixelStorage, int));

  void OnIncomingCapturedBuffer(Buffer buffer,
                                const VideoCaptureFormat& format,
                                base::TimeTicks reference_,
                                base::TimeDelta timestamp) override {}

  void OnIncomingCapturedBufferExt(
      Buffer buffer,
      const VideoCaptureFormat& format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      gfx::Rect visible_rect,
      const VideoFrameMetadata& additional_metadata) override {}

  MOCK_METHOD4(
      ResurrectLastOutputBuffer,
      Buffer(const gfx::Size&, VideoPixelFormat, VideoPixelStorage, int));

  MOCK_METHOD2(OnError, void(const base::Location&, const std::string&));

  double GetBufferPoolUtilization() const override { return 0.0; }

  MOCK_METHOD0(OnStarted, void());
};

class MockImageCaptureClient : public base::RefCounted<MockImageCaptureClient> {
 public:
  // GMock doesn't support move-only arguments, so we use this forward method.
  void DoOnGetPhotoState(mojom::PhotoStatePtr state) {
    state_ = std::move(state);
    OnCorrectGetPhotoState();
  }
  MOCK_METHOD0(OnCorrectGetPhotoState, void(void));

  const mojom::PhotoState* state() { return state_.get(); }

  MOCK_METHOD1(OnCorrectSetPhotoOptions, void(bool));

  // GMock doesn't support move-only arguments, so we use this forward method.
  void DoOnPhotoTaken(mojom::BlobPtr blob) {
    // Only PNG images are supported right now.
    EXPECT_STREQ("image/png", blob->mime_type.c_str());
    // Not worth decoding the incoming data. Just check that the header is PNG.
    // http://www.libpng.org/pub/png/spec/1.2/PNG-Rationale.html#R.PNG-file-signature
    ASSERT_GT(blob->data.size(), 4u);
    EXPECT_EQ('P', blob->data[1]);
    EXPECT_EQ('N', blob->data[2]);
    EXPECT_EQ('G', blob->data[3]);
    OnCorrectPhotoTaken();
  }
  MOCK_METHOD0(OnCorrectPhotoTaken, void(void));

 private:
  friend class base::RefCounted<MockImageCaptureClient>;
  virtual ~MockImageCaptureClient() = default;

  mojom::PhotoStatePtr state_;
};

}  // namespace

class FileVideoCaptureDeviceTest : public ::testing::Test {
 protected:
  FileVideoCaptureDeviceTest()
      : client_(new MockClient()),
        image_capture_client_(new MockImageCaptureClient()) {}

  void SetUp() override {
    EXPECT_CALL(*client_, OnError(_, _)).Times(0);
    EXPECT_CALL(*client_, OnStarted());
    device_ = std::make_unique<FileVideoCaptureDevice>(
        GetTestDataFilePath("bear.mjpeg"));
    device_->AllocateAndStart(VideoCaptureParams(), std::move(client_));
  }

  void TearDown() override { device_->StopAndDeAllocate(); }

  std::unique_ptr<MockClient> client_;
  const scoped_refptr<MockImageCaptureClient> image_capture_client_;
  std::unique_ptr<VideoCaptureDevice> device_;
  VideoCaptureFormat last_format_;
};

TEST_F(FileVideoCaptureDeviceTest, GetPhotoState) {
  VideoCaptureDevice::GetPhotoStateCallback scoped_get_callback =
      base::BindOnce(&MockImageCaptureClient::DoOnGetPhotoState,
                     image_capture_client_);

  device_->GetPhotoState(std::move(scoped_get_callback));

  const mojom::PhotoState* state = image_capture_client_->state();
  EXPECT_TRUE(state);
}

TEST_F(FileVideoCaptureDeviceTest, SetPhotoOptions) {
  mojom::PhotoSettingsPtr photo_settings = mojom::PhotoSettings::New();
  VideoCaptureDevice::SetPhotoOptionsCallback scoped_set_callback =
      base::BindOnce(&MockImageCaptureClient::OnCorrectSetPhotoOptions,
                     image_capture_client_);
  EXPECT_CALL(*image_capture_client_.get(), OnCorrectSetPhotoOptions(true))
      .Times(1);
  device_->SetPhotoOptions(std::move(photo_settings),
                           std::move(scoped_set_callback));
}

}  // namespace media