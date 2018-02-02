// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/single_client_video_capture_host.h"

#include "base/memory/weak_ptr.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

class MockVideoCaptureDevice final
    : public content::LaunchedVideoCaptureDevice {
 public:
  MockVideoCaptureDevice() {}
  ~MockVideoCaptureDevice() override {}
  void GetPhotoState(
      VideoCaptureDevice::GetPhotoStateCallback callback) const override {}
  void SetPhotoOptions(
      mojom::PhotoSettingsPtr settings,
      VideoCaptureDevice::SetPhotoOptionsCallback callback) override {}
  void TakePhoto(VideoCaptureDevice::TakePhotoCallback callback) {}
  void SetDesktopCaptureWindowIdAsync(gfx::NativeViewId window_id,
                                      base::OnceClosure done_cb) override {}
  MOCK_METHOD0(MaybeSuspendDevice, void());
  MOCK_METHOD0(ResumeDevice, void());
  MOCK_METHOD0(RequestRefreshFrame, void());
  MOCK_METHOD2(OnUtilizationReport, void(int, double));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockVideoCaptureDevice);
};

using DeviceLaunchedCallback =
    base::OnceCallback<void(base::WeakPtr<VideoFrameReceiver>,
                            MockVideoCaptureDevice*)>;

class FakeDeviceLauncher final : public content::VideoCaptureDeviceLauncher {
 public:
  explicit FakeDeviceLauncher(DeviceLaunchedCallback launched_cb)
      : after_launch_cb_(std::move(launched_cb)), weak_factory_(this) {}
  ~FakeDeviceLauncher() override {}

  // content::VideoCaptureDeviceLauncher implementation.
  void LaunchDeviceAsync(const std::string& device_id,
                         content::MediaStreamType stream_type,
                         const VideoCaptureParams& params,
                         base::WeakPtr<VideoFrameReceiver> receiver,
                         base::OnceClosure connection_lost_cb,
                         Callbacks* callbacks,
                         base::OnceClosure done_cb) override {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FakeDeviceLauncher::OnDeviceLaunched,
                                  weak_factory_.GetWeakPtr(), receiver,
                                  callbacks, std::move(done_cb)));
  }

  void AbortLaunch() override {}

 private:
  void OnDeviceLaunched(base::WeakPtr<VideoFrameReceiver> receiver,
                        VideoCaptureDeviceLauncher::Callbacks* callbacks,
                        base::OnceClosure done_cb) {
    auto launched_device = std::make_unique<MockVideoCaptureDevice>();
    EXPECT_FALSE(after_launch_cb_.is_null());
    base::ResetAndReturn(&after_launch_cb_)
        .Run(receiver, launched_device.get());
    callbacks->OnDeviceLaunched(std::move(launched_device));
    std::move(done_cb).Run();
  }

  DeviceLaunchedCallback after_launch_cb_;
  base::WeakPtrFactory<FakeDeviceLauncher> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(FakeDeviceLauncher);
};

class StubBufferHandleProvider final
    : public VideoCaptureDevice::Client::Buffer::HandleProvider {
 public:
  StubBufferHandleProvider() {}

  ~StubBufferHandleProvider() override {}

  mojo::ScopedSharedBufferHandle GetHandleForInterProcessTransit(
      bool read_only) override {
    return mojo::SharedBufferHandle::Create(10);
  }

  base::SharedMemoryHandle GetNonOwnedSharedMemoryHandleForLegacyIPC()
      override {
    NOTREACHED();
    return base::SharedMemoryHandle();
  }

  std::unique_ptr<VideoCaptureBufferHandle> GetHandleForInProcessAccess()
      override {
    NOTREACHED();
    return nullptr;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(StubBufferHandleProvider);
};

class StubReadWritePermission final
    : public VideoCaptureDevice::Client::Buffer::ScopedAccessPermission {
 public:
  StubReadWritePermission() {}
  ~StubReadWritePermission() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(StubReadWritePermission);
};

class MockVideoCaptureObserver final : public mojom::VideoCaptureObserver {
 public:
  explicit MockVideoCaptureObserver(mojom::VideoCaptureHostPtr host)
      : host_(std::move(host)), binding_(this) {}
  MOCK_METHOD1(OnBufferCreatedCall, void(int buffer_id));
  void OnBufferCreated(int32_t buffer_id,
                       mojo::ScopedSharedBufferHandle handle) override {
    EXPECT_EQ(buffers_.find(buffer_id), buffers_.end());
    EXPECT_EQ(frame_infos_.find(buffer_id), frame_infos_.end());
    buffers_[buffer_id] = std::move(handle);
    OnBufferCreatedCall(buffer_id);
  }

  MOCK_METHOD1(OnBufferReadyCall, void(int buffer_id));
  void OnBufferReady(int32_t buffer_id,
                     mojom::VideoFrameInfoPtr info) override {
    EXPECT_TRUE(buffers_.find(buffer_id) != buffers_.end());
    EXPECT_EQ(frame_infos_.find(buffer_id), frame_infos_.end());
    frame_infos_[buffer_id] = std::move(info);
    OnBufferReadyCall(buffer_id);
  }

  MOCK_METHOD1(OnBufferDestroyedCall, void(int buffer_id));
  void OnBufferDestroyed(int32_t buffer_id) override {
    // The consumer should have finished consuming the buffer before it is being
    // destroyed.
    EXPECT_TRUE(frame_infos_.find(buffer_id) == frame_infos_.end());
    const auto iter = buffers_.find(buffer_id);
    EXPECT_TRUE(iter != buffers_.end());
    buffers_.erase(iter);
    OnBufferDestroyedCall(buffer_id);
  }

  MOCK_METHOD1(OnStateChanged, void(mojom::VideoCaptureState state));

  void Start() {
    mojom::VideoCaptureObserverPtr observer;
    binding_.Bind(mojo::MakeRequest(&observer));
    host_->Start(0, 0, VideoCaptureParams(), std::move(observer));
  }

  void FinishConsumingBuffer(int32_t buffer_id, double utilization) {
    EXPECT_TRUE(buffers_.find(buffer_id) != buffers_.end());
    const auto iter = frame_infos_.find(buffer_id);
    EXPECT_TRUE(iter != frame_infos_.end());
    frame_infos_.erase(iter);
    host_->ReleaseBuffer(0, buffer_id, utilization);
  }

  void Stop() { host_->Stop(0); }

 private:
  mojom::VideoCaptureHostPtr host_;
  mojo::Binding<mojom::VideoCaptureObserver> binding_;
  base::flat_map<int, mojo::ScopedSharedBufferHandle> buffers_;
  base::flat_map<int, mojom::VideoFrameInfoPtr> frame_infos_;

  DISALLOW_COPY_AND_ASSIGN(MockVideoCaptureObserver);
};

mojom::VideoFrameInfoPtr GetVideoFrameInfo() {
  return mojom::VideoFrameInfo::New(base::TimeDelta(),
                                    std::make_unique<base::DictionaryValue>(),
                                    PIXEL_FORMAT_I420, VideoPixelStorage::CPU,
                                    gfx::Size(320, 180), gfx::Rect(320, 180));
}

}  // namespace

class SingleClientVideoCaptureHostTest : public ::testing::Test {
 public:
  SingleClientVideoCaptureHostTest() : weak_factory_(this) {
    auto host_impl = std::make_unique<SingleClientVideoCaptureHost>(
        std::string(), content::MediaStreamType::MEDIA_TAB_VIDEO_CAPTURE,
        VideoCaptureParams(),
        base::BindRepeating(
            &SingleClientVideoCaptureHostTest::CreateDeviceLauncher,
            base::Unretained(this)));
    mojom::VideoCaptureHostPtr host;
    mojo::MakeStrongBinding(std::move(host_impl), mojo::MakeRequest(&host));
    consumer_ = std::make_unique<MockVideoCaptureObserver>(std::move(host));
    EXPECT_CALL(*this, OnDeviceLaunchedCall()).Times(1);
    consumer_->Start();
    content::RunAllPendingInMessageLoop();

    // The video capture device is launched.
    EXPECT_TRUE(!!launched_device_);
    EXPECT_TRUE(!!frame_receiver_);
  }

  ~SingleClientVideoCaptureHostTest() override {
    EXPECT_CALL(*consumer_, OnStateChanged(mojom::VideoCaptureState::ENDED))
        .Times(1);
    consumer_->Stop();
    content::RunAllPendingInMessageLoop();
  }

 protected:
  std::unique_ptr<MockVideoCaptureObserver> consumer_;
  base::WeakPtr<VideoFrameReceiver> frame_receiver_;
  MockVideoCaptureDevice* launched_device_ = nullptr;

 private:
  std::unique_ptr<content::VideoCaptureDeviceLauncher> CreateDeviceLauncher() {
    return std::make_unique<FakeDeviceLauncher>(
        base::BindOnce(&SingleClientVideoCaptureHostTest::OnDeviceLaunched,
                       weak_factory_.GetWeakPtr()));
  }

  MOCK_METHOD0(OnDeviceLaunchedCall, void());
  void OnDeviceLaunched(base::WeakPtr<VideoFrameReceiver> receiver,
                        MockVideoCaptureDevice* launched_device) {
    frame_receiver_ = receiver;
    launched_device_ = launched_device;
    OnDeviceLaunchedCall();
  }

  // Run tests on the testing Browser::UI thread to conveniently run all pending
  // tasks by calling content::RunAllPendingInMessageLoop().
  content::TestBrowserThreadBundle threads_;

  base::WeakPtrFactory<SingleClientVideoCaptureHostTest> weak_factory_;
};

TEST_F(SingleClientVideoCaptureHostTest, Basic) {
  // Create two new buffers.
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(0)).Times(1);
  frame_receiver_->OnNewBufferHandle(
      0, std::make_unique<StubBufferHandleProvider>());
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(1)).Times(1);
  frame_receiver_->OnNewBufferHandle(
      1, std::make_unique<StubBufferHandleProvider>());
  content::RunAllPendingInMessageLoop();

  // Two frames are ready for consuming.
  EXPECT_CALL(*consumer_, OnBufferReadyCall(0)).Times(1);
  frame_receiver_->OnFrameReadyInBuffer(
      0, 3, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferReadyCall(1)).Times(1);
  frame_receiver_->OnFrameReadyInBuffer(
      1, 5, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  content::RunAllPendingInMessageLoop();

  // Expect to receive the utilization report after consuming the first frame.
  EXPECT_CALL(*launched_device_, OnUtilizationReport(3, 1.0)).Times(1);
  consumer_->FinishConsumingBuffer(0, 1.0);
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(0)).Times(1);
  frame_receiver_->OnBufferRetired(0);
  content::RunAllPendingInMessageLoop();

  // Expect to receive the utilization report after consuming the second frame.
  EXPECT_CALL(*launched_device_, OnUtilizationReport(5, 0.5)).Times(1);
  consumer_->FinishConsumingBuffer(1, 0.5);
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(1)).Times(1);
  frame_receiver_->OnBufferRetired(1);
  content::RunAllPendingInMessageLoop();
}

TEST_F(SingleClientVideoCaptureHostTest, ReuseBufferId) {
  // Create two new buffers.
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(0)).Times(1);
  frame_receiver_->OnNewBufferHandle(
      0, std::make_unique<StubBufferHandleProvider>());
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(1)).Times(1);
  frame_receiver_->OnNewBufferHandle(
      1, std::make_unique<StubBufferHandleProvider>());
  content::RunAllPendingInMessageLoop();

  // Two frames are ready for consuming.
  EXPECT_CALL(*consumer_, OnBufferReadyCall(0)).Times(1);
  frame_receiver_->OnFrameReadyInBuffer(
      0, 3, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferReadyCall(1)).Times(1);
  frame_receiver_->OnFrameReadyInBuffer(
      1, 5, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  content::RunAllPendingInMessageLoop();

  // Retire buffer 0. The consumer is not expected to receive OnBufferDestroyed
  // since the buffer is not returned yet.
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(0)).Times(0);
  frame_receiver_->OnBufferRetired(0);
  content::RunAllPendingInMessageLoop();
  testing::Mock::VerifyAndClearExpectations(consumer_.get());

  // The buffer id known to the capture device is 0, buf the buffer id known to
  // the consumer is 2.
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(2)).Times(1);
  frame_receiver_->OnNewBufferHandle(
      0, std::make_unique<StubBufferHandleProvider>());
  content::RunAllPendingInMessageLoop();
  EXPECT_CALL(*consumer_, OnBufferReadyCall(2)).Times(1);
  frame_receiver_->OnFrameReadyInBuffer(
      0, 7, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  content::RunAllPendingInMessageLoop();

  // The retired buffer is expected to be destroyed after consumer finish
  // consuming the frame.
  EXPECT_CALL(*launched_device_, OnUtilizationReport(3, 1.0)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(0)).Times(1);
  consumer_->FinishConsumingBuffer(0, 1.0);
  content::RunAllPendingInMessageLoop();

  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(1)).Times(0);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(2)).Times(0);
  frame_receiver_->OnBufferRetired(0);
  content::RunAllPendingInMessageLoop();
  frame_receiver_->OnBufferRetired(1);
  content::RunAllPendingInMessageLoop();
  testing::Mock::VerifyAndClearExpectations(consumer_.get());

  // Both buffers are expected to be destroyed after the consumer returns them.
  EXPECT_CALL(*launched_device_, OnUtilizationReport(5, 0.5)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(1)).Times(1);
  consumer_->FinishConsumingBuffer(1, 0.5);
  content::RunAllPendingInMessageLoop();

  EXPECT_CALL(*launched_device_, OnUtilizationReport(7, 0.8)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(2)).Times(1);
  consumer_->FinishConsumingBuffer(2, 0.8);
  content::RunAllPendingInMessageLoop();
}

}  // namespace media
