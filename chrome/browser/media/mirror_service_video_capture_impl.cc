// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_video_capture_impl.h"

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "media/capture/video/video_capture_buffer_pool.h"
#include "mojo/common/values_struct_traits.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using content::BrowserThread;

namespace media {

namespace {
void DeviceLaunchCompleted() {}
}  // namespace

MirrorServiceVideoCaptureImpl::MirrorServiceVideoCaptureImpl(
    const std::string& device_id,
    content::MediaStreamType type,
    const VideoCaptureParams& params,
    scoped_refptr<base::SingleThreadTaskRunner> device_task_runner)
    : device_id_(device_id),
      type_(type),
      params_(params),
      device_task_runner_(device_task_runner),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

MirrorServiceVideoCaptureImpl::~MirrorServiceVideoCaptureImpl() {}

void MirrorServiceVideoCaptureImpl::Start(
    int32_t device_id,
    int32_t session_id,
    const media::VideoCaptureParams& params,
    mojom::VideoCaptureObserverPtr observer) {
  DVLOG(1) << __func__;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  observer_ = std::move(observer);
  if (!observer_)
    return;

  if (device_launcher_) {
    VLOG(1) << "Cannot start multiple video captures.";
    observer_->OnStateChanged(mojom::VideoCaptureState::FAILED);
    return;
  }

  // Start a video capture device.
  device_launcher_ = content::VideoCaptureDeviceLauncher::
      CreateInProcessVideoCaptureDeviceLauncher(device_task_runner_);
  device_launcher_->LaunchDeviceAsync(
      device_id_, type_, params_, weak_factory_.GetWeakPtr(),
      base::BindOnce(&MirrorServiceVideoCaptureImpl::OnError,
                     weak_factory_.GetWeakPtr()),
      this, base::BindOnce(&DeviceLaunchCompleted));
}

void MirrorServiceVideoCaptureImpl::Stop(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << __func__;

  // Returns all the buffers.
  for (const auto& entry : buffer_context_map_) {
    OnFinishedConsumingBuffer(
        entry.first,
        VideoFrameConsumerFeedbackObserver::kNoUtilizationRecorded);
  }
  observer_->OnStateChanged(mojom::VideoCaptureState::ENDED);
  encounted_error_ = false;
  launched_device_ = nullptr;
  device_launcher_ = nullptr;
}

void MirrorServiceVideoCaptureImpl::Pause(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!launched_device_) {
    NOTREACHED() << "No launched device while pausing capture.";
    return;
  }
  launched_device_->MaybeSuspendDevice();
}

void MirrorServiceVideoCaptureImpl::Resume(
    int32_t device_id,
    int32_t session_id,
    const media::VideoCaptureParams& params) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!launched_device_) {
    NOTREACHED() << "No launched device while resuming capture.";
    return;
  }
  launched_device_->ResumeDevice();
}

void MirrorServiceVideoCaptureImpl::RequestRefreshFrame(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!launched_device_) {
    NOTREACHED() << "No launched device while requesting refresh frame.";
    return;
  }
  launched_device_->RequestRefreshFrame();
}

void MirrorServiceVideoCaptureImpl::ReleaseBuffer(
    int32_t device_id,
    int32_t buffer_id,
    double consumer_resource_utilization) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(3) << __func__ << ": buffer_id=" << buffer_id;

  OnFinishedConsumingBuffer(buffer_id, consumer_resource_utilization);
}

void MirrorServiceVideoCaptureImpl::GetDeviceSupportedFormats(
    int32_t device_id,
    int32_t session_id,
    GetDeviceSupportedFormatsCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  NOTREACHED();
  std::move(callback).Run(VideoCaptureFormats());
}

void MirrorServiceVideoCaptureImpl::GetDeviceFormatsInUse(
    int32_t device_id,
    int32_t session_id,
    GetDeviceFormatsInUseCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  NOTREACHED();
  std::move(callback).Run(VideoCaptureFormats());
}

void MirrorServiceVideoCaptureImpl::OnNewBufferHandle(
    int buffer_id,
    std::unique_ptr<Buffer::HandleProvider> handle_provider) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(3) << __func__ << ": buffer_id=" << buffer_id;
  DCHECK(observer_);
  DCHECK_NE(buffer_id, VideoCaptureBufferPool::kInvalidId);
  DCHECK(id_map_.find(buffer_id) == id_map_.end());
  id_map_.emplace(std::make_pair(buffer_id, next_buffer_context_id_));
  observer_->OnBufferCreated(
      next_buffer_context_id_++,
      handle_provider->GetHandleForInterProcessTransit(true /* read only */));
}

void MirrorServiceVideoCaptureImpl::OnFrameReadyInBuffer(
    int buffer_id,
    int frame_feedback_id,
    std::unique_ptr<Buffer::ScopedAccessPermission> buffer_read_permission,
    mojom::VideoFrameInfoPtr frame_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(3) << __func__ << ": buffer_id=" << buffer_id;
  DCHECK(observer_);
  const auto id_iter = id_map_.find(buffer_id);
  DCHECK(id_iter != id_map_.end());

  if (encounted_error_)
    return;

  const int buffer_context_id = id_iter->second;
  DCHECK(buffer_context_map_.find(buffer_context_id) ==
         buffer_context_map_.end());
  buffer_context_map_.emplace(std::make_pair(
      buffer_context_id,
      std::make_pair(frame_feedback_id, std::move(buffer_read_permission))));
  observer_->OnBufferReady(buffer_context_id, std::move(frame_info));
}

void MirrorServiceVideoCaptureImpl::OnBufferRetired(int buffer_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(3) << __func__ << ": buffer_id=" << buffer_id;

  const auto id_iter = id_map_.find(buffer_id);
  DCHECK(id_iter != id_map_.end());
  const int buffer_context_id = id_iter->second;
  id_map_.erase(id_iter);
  if (buffer_context_map_.find(buffer_context_id) !=
      buffer_context_map_.end()) {
    // The consumer is still using the buffer. The BufferContext needs to be
    // held until the consumer finishes.
    DCHECK(retired_buffers_.find(buffer_context_id) == retired_buffers_.end());
    retired_buffers_.insert(buffer_context_id);
  }
}

void MirrorServiceVideoCaptureImpl::OnError() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  VLOG(1) << __func__;
  encounted_error_ = true;
  if (observer_)
    observer_->OnStateChanged(mojom::VideoCaptureState::FAILED);
  buffer_context_map_.clear();
  launched_device_ = nullptr;
  device_launcher_ = nullptr;
}

void MirrorServiceVideoCaptureImpl::OnLog(const std::string& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  VLOG(3) << message;
}

void MirrorServiceVideoCaptureImpl::OnStarted() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(2) << __func__;
  DCHECK(observer_);
  observer_->OnStateChanged(mojom::VideoCaptureState::STARTED);
}

void MirrorServiceVideoCaptureImpl::OnStartedUsingGpuDecode() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();
}

void MirrorServiceVideoCaptureImpl::OnDeviceLaunched(
    std::unique_ptr<content::LaunchedVideoCaptureDevice> device) {
  DVLOG(1) << __func__;
  launched_device_ = std::move(device);
}

void MirrorServiceVideoCaptureImpl::OnDeviceLaunchFailed() {
  DVLOG(1) << __func__;
  launched_device_ = nullptr;
  OnError();
}

void MirrorServiceVideoCaptureImpl::OnDeviceLaunchAborted() {
  DVLOG(1) << __func__;
  launched_device_ = nullptr;
  OnError();
}

void MirrorServiceVideoCaptureImpl::OnFinishedConsumingBuffer(
    int buffer_context_id,
    double consumer_resource_utilization) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(observer_);
  const auto buffer_context_iter = buffer_context_map_.find(buffer_context_id);
  if (buffer_context_iter == buffer_context_map_.end()) {
    // Stop() must have been called before.
    DCHECK(!launched_device_);
    return;
  }
  VideoFrameConsumerFeedbackObserver* feedback_observer =
      launched_device_.get();
  if (feedback_observer) {
    feedback_observer->OnUtilizationReport(buffer_context_iter->second.first,
                                           consumer_resource_utilization);
  } else {
    DVLOG(1) << "Warning: Null VideoFrameConsumerFeedbackObserver.";
  }
  buffer_context_map_.erase(buffer_context_iter);
  const auto retired_iter = retired_buffers_.find(buffer_context_id);
  if (retired_iter != retired_buffers_.end()) {
    retired_buffers_.erase(retired_iter);
    observer_->OnBufferDestroyed(buffer_context_id);
  }
}

}  // namespace media
