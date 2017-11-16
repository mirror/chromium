// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "media/capture/video/video_capture_device_info.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/video_capture/test/mock_producer.h"
#include "services/video_capture/test/mock_receiver.h"
#include "services/video_capture/virtual_device_mojo_adapter.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Mock;
using testing::InvokeWithoutArgs;

namespace video_capture {

namespace {

const std::string kTestDeviceId = "/test/device";
const std::string kTestDeviceName = "Test Device";
const gfx::Size kTestFrameSize = {640 /* width */, 480 /* height */};
const media::VideoPixelFormat kTestPixelFormat =
    media::VideoPixelFormat::PIXEL_FORMAT_I420;
const media::VideoPixelStorage kTestPixelStorage =
    media::VideoPixelStorage::PIXEL_STORAGE_CPU;

}  // anonymous namespace

class VirtualDeviceTest : public ::testing::Test {
 public:
  VirtualDeviceTest() : ref_factory_(base::Bind(&base::DoNothing)) {}
  ~VirtualDeviceTest() override {}

  void SetUp() override {
    device_info_.descriptor.device_id = kTestDeviceId;
    device_info_.descriptor.display_name = kTestDeviceName;
    mojom::ProducerPtr producer_proxy;
    producer_ =
        std::make_unique<MockProducer>(mojo::MakeRequest(&producer_proxy));
    device_adapter_ = std::make_unique<VirtualDeviceMojoAdapter>(
        ref_factory_.CreateRef(), device_info_, std::move(producer_proxy));
  }

  void OnFrameBufferReceived(bool valid_buffer_expected, int32_t buffer_id) {
    if (!valid_buffer_expected) {
      EXPECT_EQ(-1, buffer_id);
      return;
    }

    EXPECT_NE(-1, buffer_id);
    received_buffer_ids_.push_back(buffer_id);
  }

  void VerifyAndGetMaxFrameBuffers() {
    // Should receive valid buffer for up to the maximum buffer count.
    for (int i = 0;
         i < VirtualDeviceMojoAdapter::max_buffer_pool_buffer_count(); i++) {
      device_adapter_->GetFrameBuffer(
          kTestFrameSize, kTestPixelFormat, kTestPixelStorage,
          base::Bind(&VirtualDeviceTest::OnFrameBufferReceived,
                     base::Unretained(this), true /* valid_buffer_expected */));
    }
    EXPECT_EQ(VirtualDeviceMojoAdapter::max_buffer_pool_buffer_count(),
              static_cast<int>(received_buffer_ids_.size()));

    // No more buffer available. Invalid buffer id should be returned.
    device_adapter_->GetFrameBuffer(
        kTestFrameSize, kTestPixelFormat, kTestPixelStorage,
        base::Bind(&VirtualDeviceTest::OnFrameBufferReceived,
                   base::Unretained(this), false /* valid_buffer_expected */));
  }

 protected:
  std::unique_ptr<VirtualDeviceMojoAdapter> device_adapter_;
  // ID of buffers received and owned by the producer.
  std::vector<int> received_buffer_ids_;
  std::unique_ptr<MockProducer> producer_;

 private:
  base::MessageLoop loop_;
  service_manager::ServiceContextRefFactory ref_factory_;
  media::VideoCaptureDeviceInfo device_info_;
};

TEST_F(VirtualDeviceTest, VerifyDeviceInfo) {
  EXPECT_EQ(kTestDeviceId, device_adapter_->device_info().descriptor.device_id);
  EXPECT_EQ(kTestDeviceName,
            device_adapter_->device_info().descriptor.display_name);
}

TEST_F(VirtualDeviceTest, OnFrameReadyInBufferWithoutReceiver) {
  // Obtain maximum number of buffers.
  VerifyAndGetMaxFrameBuffers();

  // Release one buffer back to the pool, no consumer hold since there is no
  // receiver.
  device_adapter_->OnFrameReadyInBuffer(received_buffer_ids_.at(0),
                                        media::mojom::VideoFrameInfo::New());

  // Verify there is a buffer available now.
  device_adapter_->GetFrameBuffer(
      kTestFrameSize, kTestPixelFormat, kTestPixelStorage,
      base::Bind(&VirtualDeviceTest::OnFrameBufferReceived,
                 base::Unretained(this), true /* valid_buffer_expected */));
}

TEST_F(VirtualDeviceTest, OnFrameReadyInBufferWithReceiver) {
  base::RunLoop wait_loop;

  // Obtain maximum number of buffers.
  EXPECT_CALL(*producer_, DoOnNewBufferHandle(_, _))
      .Times(VirtualDeviceMojoAdapter::max_buffer_pool_buffer_count());
  VerifyAndGetMaxFrameBuffers();

  mojom::ReceiverPtr receiver_proxy;
  MockReceiver receiver(mojo::MakeRequest(&receiver_proxy));
  int num_frames_arrived = 0;
  EXPECT_CALL(receiver, OnStarted());
  EXPECT_CALL(receiver, DoOnNewBufferHandle(_, _))
      .Times(VirtualDeviceMojoAdapter::max_buffer_pool_buffer_count());
  EXPECT_CALL(receiver, DoOnFrameReadyInBuffer(_, _, _, _))
      .WillRepeatedly(InvokeWithoutArgs([&wait_loop, &num_frames_arrived]() {
        num_frames_arrived += 1;
        if (num_frames_arrived >=
            VirtualDeviceMojoAdapter::max_buffer_pool_buffer_count()) {
          wait_loop.Quit();
        }
      }));
  device_adapter_->Start(media::VideoCaptureParams(),
                         std::move(receiver_proxy));
  for (auto buffer_id : received_buffer_ids_) {
    media::mojom::VideoFrameInfoPtr info = media::mojom::VideoFrameInfo::New();
    // |info->metadata| cannot be a nullptr when going over mojo boundary.
    info->metadata = std::make_unique<base::DictionaryValue>();
    device_adapter_->OnFrameReadyInBuffer(buffer_id, std::move(info));
  }
  wait_loop.Run();
}

}  // namespace video_capture
