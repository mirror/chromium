// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_video_capture_impl.h"

#include "base/test/scoped_task_environment.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/binding.h"
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
  explicit MockVideoCaptureObserver(mojom::VideoCaptureObserverRequest request)
      : binding_(this, std::move(request)) {}
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

  void FinishConsumingBuffer(int32_t buffer_id) {
    EXPECT_TRUE(buffers_.find(buffer_id) != buffers_.end());
    const auto iter = frame_infos_.find(buffer_id);
    EXPECT_TRUE(iter != frame_infos_.end());
    frame_infos_.erase(iter);
  }

  MOCK_METHOD1(OnStateChanged, void(mojom::VideoCaptureState state));

 private:
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

class MirrorServiceVideoCaptureImplTest : public ::testing::Test {
 public:
  MirrorServiceVideoCaptureImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        browser_thread_bundle_(1) {
    capture_impl_ = std::make_unique<MirrorServiceVideoCaptureImpl>(
        std::string(), content::MediaStreamType::MEDIA_TAB_VIDEO_CAPTURE,
        VideoCaptureParams(),
        scoped_task_environment_.GetMainThreadTaskRunner());
    mojom::VideoCaptureObserverPtr observer;
    consumer_ = std::make_unique<MockVideoCaptureObserver>(
        mojo::MakeRequest(&observer));
    capture_impl_->observer_ = std::move(observer);
    auto launched_device = std::make_unique<MockVideoCaptureDevice>();
    launched_device_ = launched_device.get();
    capture_impl_->OnDeviceLaunched(std::move(launched_device));
  }
  ~MirrorServiceVideoCaptureImplTest() override {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MirrorServiceVideoCaptureImpl> capture_impl_;
  std::unique_ptr<MockVideoCaptureObserver> consumer_;
  MockVideoCaptureDevice* launched_device_;

 private:
  content::TestBrowserThreadBundle browser_thread_bundle_;
};

TEST_F(MirrorServiceVideoCaptureImplTest, Basic) {
  // Create two new buffers.
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(0)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(1)).Times(1);
  capture_impl_->OnNewBufferHandle(
      0, std::make_unique<StubBufferHandleProvider>());
  capture_impl_->OnNewBufferHandle(
      1, std::make_unique<StubBufferHandleProvider>());
  scoped_task_environment_.RunUntilIdle();

  // Two frames are ready for consuming.
  EXPECT_CALL(*consumer_, OnBufferReadyCall(0)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferReadyCall(1)).Times(1);
  capture_impl_->OnFrameReadyInBuffer(
      0, 3, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  capture_impl_->OnFrameReadyInBuffer(
      1, 5, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  scoped_task_environment_.RunUntilIdle();

  // Expect to receive the utilization report after consuming the first frame.
  consumer_->FinishConsumingBuffer(0);
  EXPECT_CALL(*launched_device_, OnUtilizationReport(3, 1.0)).Times(1);
  capture_impl_->ReleaseBuffer(0, 0, 1.0);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(0)).Times(1);
  capture_impl_->OnBufferRetired(0);
  scoped_task_environment_.RunUntilIdle();

  // Expect to receive the utilization report after consuming the second frame.
  consumer_->FinishConsumingBuffer(1);
  EXPECT_CALL(*launched_device_, OnUtilizationReport(5, 0.5)).Times(1);
  capture_impl_->ReleaseBuffer(0, 1, 0.5);
  scoped_task_environment_.RunUntilIdle();
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(1)).Times(1);
  capture_impl_->OnBufferRetired(1);
  scoped_task_environment_.RunUntilIdle();
}

TEST_F(MirrorServiceVideoCaptureImplTest, ReuseBufferId) {
  // Create two new buffers.
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(0)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(1)).Times(1);
  capture_impl_->OnNewBufferHandle(
      0, std::make_unique<StubBufferHandleProvider>());
  scoped_task_environment_.RunUntilIdle();
  capture_impl_->OnNewBufferHandle(
      1, std::make_unique<StubBufferHandleProvider>());
  scoped_task_environment_.RunUntilIdle();

  // Two frames are ready for consuming.
  EXPECT_CALL(*consumer_, OnBufferReadyCall(0)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferReadyCall(1)).Times(1);
  capture_impl_->OnFrameReadyInBuffer(
      0, 3, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  scoped_task_environment_.RunUntilIdle();
  capture_impl_->OnFrameReadyInBuffer(
      1, 5, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  scoped_task_environment_.RunUntilIdle();

  // Retire buffer 0. The consumer is not expected to receive OnBufferDestroyed
  // since the buffer is not returned yet.
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(0)).Times(0);
  capture_impl_->OnBufferRetired(0);
  scoped_task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(consumer_.get());

  // The buffer id known to the capture device is 0, buf the buffer id known to
  // the consumer is 2.
  EXPECT_CALL(*consumer_, OnBufferCreatedCall(2)).Times(1);
  capture_impl_->OnNewBufferHandle(
      0, std::make_unique<StubBufferHandleProvider>());
  scoped_task_environment_.RunUntilIdle();
  EXPECT_CALL(*consumer_, OnBufferReadyCall(2)).Times(1);
  capture_impl_->OnFrameReadyInBuffer(
      0, 7, std::make_unique<StubReadWritePermission>(), GetVideoFrameInfo());
  scoped_task_environment_.RunUntilIdle();

  // The retired buffer is expected to be destroyed after consumer finish
  // consuming the frame.
  consumer_->FinishConsumingBuffer(0);
  EXPECT_CALL(*launched_device_, OnUtilizationReport(3, 1.0)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(0)).Times(1);
  capture_impl_->ReleaseBuffer(0, 0, 1.0);
  scoped_task_environment_.RunUntilIdle();

  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(1)).Times(0);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(2)).Times(0);
  capture_impl_->OnBufferRetired(0);
  scoped_task_environment_.RunUntilIdle();
  capture_impl_->OnBufferRetired(1);
  scoped_task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(consumer_.get());

  // Both buffers are expected to be destroyed after the consumer returns them.
  consumer_->FinishConsumingBuffer(1);
  EXPECT_CALL(*launched_device_, OnUtilizationReport(5, 0.5)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(1)).Times(1);
  capture_impl_->ReleaseBuffer(0, 1, 0.5);
  scoped_task_environment_.RunUntilIdle();

  consumer_->FinishConsumingBuffer(2);
  EXPECT_CALL(*launched_device_, OnUtilizationReport(7, 0.8)).Times(1);
  EXPECT_CALL(*consumer_, OnBufferDestroyedCall(2)).Times(1);
  capture_impl_->ReleaseBuffer(0, 2, 0.8);
  scoped_task_environment_.RunUntilIdle();
}

}  // namespace media
