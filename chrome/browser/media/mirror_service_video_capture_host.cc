// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_video_capture_host.h"

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/video_capture_device_launcher.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "media/capture/video/video_capture_buffer_pool.h"
#include "mojo/common/values_struct_traits.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using content::BrowserThread;

namespace media {

namespace {

void VideoCaptureDeviceLaunchDone() {
  DVLOG(1) << __func__;
}

}  // namespace

MirrorServiceVideoCaptureHost::MirrorServiceVideoCaptureHost(
    const std::string& device_id,
    content::MediaStreamType type,
    const VideoCaptureParams& params,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner)
    : device_id_(device_id),
      type_(type),
      params_(params),
      audio_task_runner_(audio_task_runner),
      weak_factory_(this) {}

MirrorServiceVideoCaptureHost::~MirrorServiceVideoCaptureHost() {}

// static
void MirrorServiceVideoCaptureHost::Create(
    const std::string& device_id,
    content::MediaStreamType type,
    const VideoCaptureParams& params,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    base::OnceCallback<void(content::mojom::VideoCaptureHostPtr)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  content::mojom::VideoCaptureHostPtr ptr;
  content::mojom::VideoCaptureHostRequest request = mojo::MakeRequest(&ptr);
  mojo::MakeStrongBinding(base::MakeUnique<MirrorServiceVideoCaptureHost>(
                              device_id, type, params, audio_task_runner),
                          std::move(request));
  if (!callback.is_null())
    base::ResetAndReturn(&callback).Run(std::move(ptr));
}

void MirrorServiceVideoCaptureHost::Start(
    int32_t device_id,
    int32_t session_id,
    const media::VideoCaptureParams& params,
    content::mojom::VideoCaptureObserverPtr observer) {
  DVLOG(1) << __func__;
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  observer_ = std::move(observer);
  if (!observer_)
    return;

  if (device_launcher_) {
    VLOG(1) << "Cannot start multiple video captures.";
    observer_->OnStateChanged(content::mojom::VideoCaptureState::FAILED);
    return;
  }

  DCHECK(audio_task_runner_.get());
  // Start a video capture device.
  device_launcher_ = content::VideoCaptureDeviceLauncher::
      CreateInProcessVideoCaptureDeviceLauncher(audio_task_runner_);
  device_launcher_->LaunchDeviceAsync(
      device_id_, type_, params_, weak_factory_.GetWeakPtr(),
      base::BindOnce(&MirrorServiceVideoCaptureHost::OnError,
                     weak_factory_.GetWeakPtr()),
      this, base::BindOnce(&VideoCaptureDeviceLaunchDone));
}

void MirrorServiceVideoCaptureHost::Stop(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << __func__;

  for (const auto& buffer_id : buffers_in_use_) {
    OnFinishedConsumingBuffer(
        buffer_id, VideoFrameConsumerFeedbackObserver::kNoUtilizationRecorded);
  }

  buffers_in_use_.clear();
  state_ = CaptureState::kEnded;
  observer_->OnStateChanged(content::mojom::VideoCaptureState::ENDED);
}

void MirrorServiceVideoCaptureHost::Pause(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!launched_device_) {
    NOTREACHED() << "Got Null controller while pausing capture.";
    return;
  }

  launched_device_->MaybeSuspendDevice();
}

void MirrorServiceVideoCaptureHost::Resume(
    int32_t device_id,
    int32_t session_id,
    const media::VideoCaptureParams& params) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  DCHECK(launched_device_);
  launched_device_->ResumeDevice();
}

void MirrorServiceVideoCaptureHost::RequestRefreshFrame(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!launched_device_) {
    NOTREACHED() << "Got null controller while requesting refresh frame.";
    return;
  }

  launched_device_->RequestRefreshFrame();
}

void MirrorServiceVideoCaptureHost::ReleaseBuffer(
    int32_t device_id,
    int32_t buffer_id,
    double consumer_resource_utilization) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  auto buffers_in_use_entry_iter = std::find(
      std::begin(buffers_in_use_), std::end(buffers_in_use_), buffer_id);
  if (buffers_in_use_entry_iter == std::end(buffers_in_use_)) {
    NOTREACHED();
    return;
  }
  buffers_in_use_.erase(buffers_in_use_entry_iter);
  OnFinishedConsumingBuffer(buffer_id, consumer_resource_utilization);
}

void MirrorServiceVideoCaptureHost::GetDeviceSupportedFormats(
    int32_t device_id,
    int32_t session_id,
    GetDeviceSupportedFormatsCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  NOTREACHED();
  std::move(callback).Run(VideoCaptureFormats());
}

void MirrorServiceVideoCaptureHost::GetDeviceFormatsInUse(
    int32_t device_id,
    int32_t session_id,
    GetDeviceFormatsInUseCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  NOTREACHED();
  std::move(callback).Run(VideoCaptureFormats());
}

void MirrorServiceVideoCaptureHost::OnNewBufferHandle(
    int buffer_id,
    std::unique_ptr<Buffer::HandleProvider> handle_provider) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_NE(buffer_id, VideoCaptureBufferPool::kInvalidId);
  DCHECK(FindUnretiredBufferContextFromBufferId(buffer_id) == buffers_.end());

  buffers_.emplace_back(
      next_buffer_context_id_++, buffer_id, launched_device_.get(),
      handle_provider->GetHandleForInterProcessTransit(true /* read only */));
  observer_->OnBufferCreated(buffers_.back().buffer_context_id(),
                             buffers_.back().CloneHandle());
}

void MirrorServiceVideoCaptureHost::OnFrameReadyInBuffer(
    int buffer_id,
    int frame_feedback_id,
    std::unique_ptr<Buffer::ScopedAccessPermission> buffer_read_permission,
    mojom::VideoFrameInfoPtr frame_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto buffer_context_iter = FindUnretiredBufferContextFromBufferId(buffer_id);
  DCHECK(buffer_context_iter != buffers_.end());
  buffer_context_iter->set_frame_feedback_id(frame_feedback_id);
  DCHECK(!buffer_context_iter->HasConsumers());

  if (state_ != CaptureState::kError) {
    const int buffer_context_id = buffer_context_iter->buffer_context_id();
    if (!base::ContainsValue(buffers_in_use_, buffer_context_id)) {
      buffers_in_use_.push_back(buffer_context_id);
    }
    buffer_context_iter->IncreaseConsumerCount();
    observer_->OnBufferReady(buffer_context_id, frame_info.Clone());
  }
  if (buffer_context_iter->HasConsumers()) {
    buffer_context_iter->set_read_permission(std::move(buffer_read_permission));
  }
}

void MirrorServiceVideoCaptureHost::OnBufferRetired(int buffer_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  VLOG(1) << __func__;

  auto buffer_context_iter = FindUnretiredBufferContextFromBufferId(buffer_id);
  DCHECK(buffer_context_iter != buffers_.end());

  // If there are any clients still using the buffer, we need to allow them
  // to finish up. We need to hold on to the BufferContext entry until then,
  // because it contains the consumer hold.
  if (!buffer_context_iter->HasConsumers())
    ReleaseBufferContext(buffer_context_iter);
  else
    buffer_context_iter->set_is_retired();
}

void MirrorServiceVideoCaptureHost::OnError() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  VLOG(1) << __func__;
  state_ = CaptureState::kError;
  if (observer_)
    observer_->OnStateChanged(content::mojom::VideoCaptureState::FAILED);
  launched_device_ = nullptr;
  device_launcher_ = nullptr;
}

void MirrorServiceVideoCaptureHost::OnLog(const std::string& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  VLOG(3) << message;
}

void MirrorServiceVideoCaptureHost::OnStarted() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(2) << __func__;
  state_ = CaptureState::kStarted;
  DCHECK(observer_);
  observer_->OnStateChanged(content::mojom::VideoCaptureState::STARTED);
}

void MirrorServiceVideoCaptureHost::OnStartedUsingGpuDecode() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();
}

void MirrorServiceVideoCaptureHost::OnDeviceLaunched(
    std::unique_ptr<content::LaunchedVideoCaptureDevice> device) {
  DVLOG(1) << __func__;
  launched_device_ = std::move(device);
}

void MirrorServiceVideoCaptureHost::OnDeviceLaunchFailed() {
  DVLOG(1) << __func__;
  launched_device_ = nullptr;
  OnError();
}

void MirrorServiceVideoCaptureHost::OnDeviceLaunchAborted() {
  DVLOG(1) << __func__;
  launched_device_ = nullptr;
  OnError();
}

std::vector<VideoCaptureBufferContext>::iterator
MirrorServiceVideoCaptureHost::FindBufferContextFromBufferContextId(
    int buffer_context_id) {
  return std::find_if(
      buffers_.begin(), buffers_.end(),
      [buffer_context_id](const VideoCaptureBufferContext& entry) {
        return entry.buffer_context_id() == buffer_context_id;
      });
}

std::vector<VideoCaptureBufferContext>::iterator
MirrorServiceVideoCaptureHost::FindUnretiredBufferContextFromBufferId(
    int buffer_id) {
  return std::find_if(buffers_.begin(), buffers_.end(),
                      [buffer_id](const VideoCaptureBufferContext& entry) {
                        return (entry.buffer_id() == buffer_id) &&
                               (entry.is_retired() == false);
                      });
}

void MirrorServiceVideoCaptureHost::OnFinishedConsumingBuffer(
    int buffer_context_id,
    double consumer_resource_utilization) {
  auto buffer_context_iter =
      FindBufferContextFromBufferContextId(buffer_context_id);
  DCHECK(buffer_context_iter != buffers_.end());

  buffer_context_iter->RecordConsumerUtilization(consumer_resource_utilization);
  buffer_context_iter->DecreaseConsumerCount();
  if (!buffer_context_iter->HasConsumers() &&
      buffer_context_iter->is_retired()) {
    ReleaseBufferContext(buffer_context_iter);
  }
}

void MirrorServiceVideoCaptureHost::ReleaseBufferContext(
    const std::vector<VideoCaptureBufferContext>::iterator&
        buffer_context_iter) {
  observer_->OnBufferDestroyed(buffer_context_iter->buffer_context_id());
  buffers_.erase(buffer_context_iter);
}

}  // namespace media
