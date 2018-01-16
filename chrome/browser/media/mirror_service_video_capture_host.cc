// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/mirror_service_video_capture_host.h"

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/renderer_host/media/in_process_video_capture_provider.h"
#include "content/browser/renderer_host/media/video_capture_controller.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents_media_capture_id.h"
#include "mojo/common/values_struct_traits.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using content::BrowserThread;

namespace media {

namespace {

void SendVideoCaptureLogMessage(const std::string& message) {
  DVLOG(1) << "video capture: " << message;
}

void OnVideoCaptureDeviceStarted() {
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
      audio_task_runner_(audio_task_runner) {}

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
  if (video_capture_provider_) {
    DVLOG(1) << "Cannot start multiple video captures.";
    observer_->OnStateChanged(content::mojom::VideoCaptureState::FAILED);
    return;
  }

  video_capture_provider_ =
      content::InProcessVideoCaptureProvider::CreateInstanceForNonDeviceCapture(
          audio_task_runner_, base::BindRepeating(&SendVideoCaptureLogMessage));

  DCHECK(!video_capture_controller_);

  video_capture_controller_ = new content::VideoCaptureController(
      device_id_, type_, params_,
      video_capture_provider_->CreateDeviceLauncher(),
      base::BindRepeating(&SendVideoCaptureLogMessage));
  if (!video_capture_controller_) {
    DVLOG(1) << "video capture controller create error.";
    observer_->OnStateChanged(content::mojom::VideoCaptureState::FAILED);
    return;
  }
  video_capture_controller_->CreateAndStartDeviceAsync(
      params_, nullptr, base::BindOnce(&OnVideoCaptureDeviceStarted));
  video_capture_controller_->AddClient(device_id, this, session_id, params_);
}

void MirrorServiceVideoCaptureHost::Stop(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!video_capture_controller_) {
    NOTREACHED();
    return;
  }

  video_capture_controller_->RemoveClient(device_id, this);
  video_capture_controller_ = nullptr;
}

void MirrorServiceVideoCaptureHost::Pause(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!video_capture_controller_) {
    NOTREACHED() << "Got Null controller while pausing capture.";
    return;
  }

  video_capture_controller_->PauseClient(device_id, this);
  video_capture_controller_->MaybeSuspend();
}

void MirrorServiceVideoCaptureHost::Resume(
    int32_t device_id,
    int32_t session_id,
    const media::VideoCaptureParams& params) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!video_capture_controller_) {
    NOTREACHED() << "Got Null controller while resuming capture.";
    return;
  }

  video_capture_controller_->ResumeClient(device_id, this);
  video_capture_controller_->Resume();
}

void MirrorServiceVideoCaptureHost::RequestRefreshFrame(int32_t device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!video_capture_controller_) {
    NOTREACHED() << "Got null controller while requesting refresh frame.";
    return;
  }

  if (!video_capture_controller_->IsDeviceAlive())
    return;
  video_capture_controller_->RequestRefreshFrame();
}

void MirrorServiceVideoCaptureHost::ReleaseBuffer(
    int32_t device_id,
    int32_t buffer_id,
    double consumer_resource_utilization) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!video_capture_controller_)
    return;
  video_capture_controller_->ReturnBuffer(device_id, this, buffer_id,
                                          consumer_resource_utilization);
}

void MirrorServiceVideoCaptureHost::GetDeviceSupportedFormats(
    int32_t device_id,
    int32_t session_id,
    GetDeviceSupportedFormatsCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "Not reached.";
  std::move(callback).Run(VideoCaptureFormats());
}

void MirrorServiceVideoCaptureHost::GetDeviceFormatsInUse(
    int32_t device_id,
    int32_t session_id,
    GetDeviceFormatsInUseCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DVLOG(1) << "Not reached.";
  std::move(callback).Run(VideoCaptureFormats());
}

void MirrorServiceVideoCaptureHost::OnError(
    content::VideoCaptureControllerID id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(observer_);
  observer_->OnStateChanged(content::mojom::VideoCaptureState::FAILED);
  video_capture_controller_ = nullptr;
}

void MirrorServiceVideoCaptureHost::OnBufferCreated(
    content::VideoCaptureControllerID id,
    mojo::ScopedSharedBufferHandle handle,
    int length,
    int buffer_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(observer_);
  observer_->OnBufferCreated(buffer_id, std::move(handle));
}

void MirrorServiceVideoCaptureHost::OnBufferDestroyed(
    content::VideoCaptureControllerID id,
    int buffer_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(observer_);
  observer_->OnBufferDestroyed(buffer_id);
}

void MirrorServiceVideoCaptureHost::OnBufferReady(
    content::VideoCaptureControllerID id,
    int buffer_id,
    const media::mojom::VideoFrameInfoPtr& frame_info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(observer_);
  observer_->OnBufferReady(buffer_id, frame_info.Clone());
}

void MirrorServiceVideoCaptureHost::OnEnded(
    content::VideoCaptureControllerID id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(observer_);
  observer_->OnStateChanged(content::mojom::VideoCaptureState::ENDED);
  video_capture_controller_ = nullptr;
}

void MirrorServiceVideoCaptureHost::OnStarted(
    content::VideoCaptureControllerID id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(observer_);
  observer_->OnStateChanged(content::mojom::VideoCaptureState::STARTED);
}

void MirrorServiceVideoCaptureHost::OnStartedUsingGpuDecode(
    content::VideoCaptureControllerID id) {}

}  // namespace media
