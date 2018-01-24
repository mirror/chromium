// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/video_capture_client.h"

#include "media/base/bind_to_current_loop.h"
#include "media/base/video_frame.h"
#include "mojo/public/cpp/system/platform_handle.h"

using content::VideoCaptureState;

namespace media {

class VideoCaptureClient::ClientBuffer
    : public base::RefCountedThreadSafe<ClientBuffer> {
 public:
  ClientBuffer(std::unique_ptr<base::SharedMemory> buffer, size_t buffer_size)
      : buffer_(std::move(buffer)), size_(buffer_size) {}

  base::SharedMemory* buffer() const { return buffer_.get(); }
  size_t buffer_size() const { return size_; }

 private:
  friend class base::RefCountedThreadSafe<ClientBuffer>;
  virtual ~ClientBuffer() {}

  const std::unique_ptr<base::SharedMemory> buffer_;
  const size_t size_;

  DISALLOW_COPY_AND_ASSIGN(ClientBuffer);
};

VideoCaptureClient::VideoCaptureClient(
    content::mojom::VideoCaptureHostPtr host,
    base::Callback<void(content::VideoCaptureState)> callback)
    : state_change_cb_(callback), observer_binding_(this), weak_factory_(this) {
  video_capture_host_info_ = host.PassInterface();
}

VideoCaptureClient::~VideoCaptureClient() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if ((state_ == content::VIDEO_CAPTURE_STATE_STARTING ||
       state_ == content::VIDEO_CAPTURE_STATE_STARTED) &&
      video_capture_host_)
    video_capture_host_->Stop(device_id_);
}

void VideoCaptureClient::OnStateChanged(
    content::mojom::VideoCaptureState state) {
  DVLOG(1) << __func__ << " state: " << state;
  DCHECK(thread_checker_.CalledOnValidThread());

  switch (state) {
    case content::mojom::VideoCaptureState::STARTED:
      state_ = content::VIDEO_CAPTURE_STATE_STARTED;
      RequestRefreshFrame();
      if (!state_change_cb_.is_null())
        state_change_cb_.Run(state_);
      break;
    case content::mojom::VideoCaptureState::STOPPED:
      state_ = content::VIDEO_CAPTURE_STATE_STOPPED;
      client_buffers_.clear();
      break;
    case content::mojom::VideoCaptureState::PAUSED:
      if (!state_change_cb_.is_null())
        state_change_cb_.Run(content::VIDEO_CAPTURE_STATE_PAUSED);
      break;
    case content::mojom::VideoCaptureState::RESUMED:
      if (!state_change_cb_.is_null())
        state_change_cb_.Run(content::VIDEO_CAPTURE_STATE_RESUMED);
      break;
    case content::mojom::VideoCaptureState::FAILED:
      state_ = content::VIDEO_CAPTURE_STATE_ERROR;
      if (!state_change_cb_.is_null())
        state_change_cb_.Run(state_);
      break;
    case content::mojom::VideoCaptureState::ENDED:
      state_ = content::VIDEO_CAPTURE_STATE_ENDED;
      if (!state_change_cb_.is_null())
        state_change_cb_.Run(state_);
      break;
  }
}

void VideoCaptureClient::OnBufferCreated(
    int32_t buffer_id,
    mojo::ScopedSharedBufferHandle handle) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(handle.is_valid());

  base::SharedMemoryHandle memory_handle;
  size_t memory_size = 0;
  bool read_only_flag = false;

  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, &memory_size, &read_only_flag);
  DCHECK_EQ(MOJO_RESULT_OK, result);
  DCHECK_GT(memory_size, 0u);

  std::unique_ptr<base::SharedMemory> shm(
      new base::SharedMemory(memory_handle, true /* read_only */));
  if (!shm->Map(memory_size)) {
    DLOG(ERROR) << "OnBufferCreated: Map failed.";
    return;
  }
  const bool inserted =
      client_buffers_
          .insert(std::make_pair(buffer_id,
                                 new ClientBuffer(std::move(shm), memory_size)))
          .second;
  DCHECK(inserted);
}

void VideoCaptureClient::OnBufferReady(int32_t buffer_id,
                                       mojom::VideoFrameInfoPtr info) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Already stopped.
  if (client_buffers_.empty())
    return;

  if (deliver_frame_cb_.is_null())
    return;  // Dropped the captured data.

  if ((info->pixel_format != media::PIXEL_FORMAT_I420 &&
       info->pixel_format != media::PIXEL_FORMAT_Y16) ||
      info->storage_type != media::PIXEL_STORAGE_CPU) {
    LOG(DFATAL) << "Wrong pixel format or storage, got pixel format:"
                << VideoPixelFormatToString(info->pixel_format)
                << ", storage:" << info->storage_type;
    GetVideoCaptureHost()->ReleaseBuffer(device_id_, buffer_id, -1.0);
    return;
  }

  base::TimeTicks reference_time;
  media::VideoFrameMetadata frame_metadata;
  frame_metadata.MergeInternalValuesFrom(*info->metadata);
  const bool success = frame_metadata.GetTimeTicks(
      media::VideoFrameMetadata::REFERENCE_TIME, &reference_time);
  DCHECK(success);

  if (first_frame_ref_time_.is_null())
    first_frame_ref_time_ = reference_time;

  // If the timestamp is not prepared, we use reference time to make a rough
  // estimate. e.g. ThreadSafeCaptureOracle::DidCaptureFrame().
  // TODO(miu): Fix upstream capturers to always set timestamp and reference
  // time. See http://crbug/618407/ for tracking.
  if (info->timestamp.is_zero())
    info->timestamp = reference_time - first_frame_ref_time_;

  // TODO(qiangchen): Change the metric name to "reference_time" and
  // "timestamp", so that we have consistent naming everywhere.
  // Used by chrome/browser/extension/api/cast_streaming/performance_test.cc
  TRACE_EVENT_INSTANT2("cast_perf_test", "OnBufferReceived",
                       TRACE_EVENT_SCOPE_THREAD, "timestamp",
                       (reference_time - base::TimeTicks()).InMicroseconds(),
                       "time_delta", info->timestamp.InMicroseconds());

  const auto& iter = client_buffers_.find(buffer_id);
  DCHECK(iter != client_buffers_.end());
  const scoped_refptr<ClientBuffer> buffer = iter->second;
  scoped_refptr<media::VideoFrame> frame =
      media::VideoFrame::WrapExternalSharedMemory(
          static_cast<media::VideoPixelFormat>(info->pixel_format),
          info->coded_size, info->visible_rect, info->visible_rect.size(),
          reinterpret_cast<uint8_t*>(buffer->buffer()->memory()),
          buffer->buffer_size(), buffer->buffer()->handle(),
          0 /* shared_memory_offset */, info->timestamp);
  if (!frame) {
    GetVideoCaptureHost()->ReleaseBuffer(device_id_, buffer_id, -1.0);
    return;
  }

  BufferFinishedCallback buffer_finished_callback = media::BindToCurrentLoop(
      base::Bind(&VideoCaptureClient::OnClientBufferFinished,
                 weak_factory_.GetWeakPtr(), buffer_id, buffer));
  frame->AddDestructionObserver(
      base::BindOnce(&VideoCaptureClient::DidFinishConsumingFrame,
                     frame->metadata(), buffer_finished_callback));

  frame->metadata()->MergeInternalValuesFrom(*info->metadata);

  deliver_frame_cb_.Run(frame, reference_time);
}

void VideoCaptureClient::OnBufferDestroyed(int32_t buffer_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << __func__;

  const auto& cb_iter = client_buffers_.find(buffer_id);
  if (cb_iter != client_buffers_.end()) {
    DCHECK(!cb_iter->second.get() || cb_iter->second->HasOneRef())
        << "Instructed to delete buffer we are still using.";
    client_buffers_.erase(cb_iter);
  }
}

void VideoCaptureClient::Start(VideoCaptureDeliverFrameCB deliver_cb) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "VideoCaptureClient: " << __func__;
  DCHECK(!deliver_cb.is_null());
  deliver_frame_cb_ = deliver_cb;

  switch (state_) {
    case content::VIDEO_CAPTURE_STATE_STARTING:
    case content::VIDEO_CAPTURE_STATE_STARTED:
      return;
    case content::VIDEO_CAPTURE_STATE_STOPPING:
      DVLOG(1) << "Start capture while stopping.";
      return;
    case content::VIDEO_CAPTURE_STATE_STOPPED:
    case content::VIDEO_CAPTURE_STATE_ENDED:
      DVLOG(1) << "Start video capture.";
      StartCaptureInternal();
      return;
    case content::VIDEO_CAPTURE_STATE_ERROR:
      if (!state_change_cb_.is_null())
        state_change_cb_.Run(content::VIDEO_CAPTURE_STATE_ERROR);
      return;
    case content::VIDEO_CAPTURE_STATE_PAUSED:
    case content::VIDEO_CAPTURE_STATE_RESUMED:
      NOTREACHED();
      return;
  }
}

void VideoCaptureClient::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());

  DVLOG(1) << "Stop video capture.";
  StopDevice();
  client_buffers_.clear();
}

void VideoCaptureClient::SuspendCapture(bool suspend) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << __func__;
  if (suspend)
    GetVideoCaptureHost()->Pause(device_id_);
  else
    GetVideoCaptureHost()->Resume(device_id_, 0, VideoCaptureParams());
}

void VideoCaptureClient::RequestRefreshFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());
  GetVideoCaptureHost()->RequestRefreshFrame(device_id_);
}

content::mojom::VideoCaptureHost* VideoCaptureClient::GetVideoCaptureHost() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!video_capture_host_.get())
    video_capture_host_.Bind(std::move(video_capture_host_info_));
  return video_capture_host_.get();
}

void VideoCaptureClient::StopDevice() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << __func__;
  if (state_ != content::VIDEO_CAPTURE_STATE_STARTING &&
      state_ != content::VIDEO_CAPTURE_STATE_STARTED)
    return;
  state_ = content::VIDEO_CAPTURE_STATE_STOPPING;
  GetVideoCaptureHost()->Stop(device_id_);
}

void VideoCaptureClient::StartCaptureInternal() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << __func__;
  state_ = content::VIDEO_CAPTURE_STATE_STARTING;

  content::mojom::VideoCaptureObserverPtr observer;
  observer_binding_.Bind(mojo::MakeRequest(&observer));
  GetVideoCaptureHost()->Start(device_id_, 0, VideoCaptureParams(),
                               std::move(observer));
}

void VideoCaptureClient::OnClientBufferFinished(
    int buffer_id,
    scoped_refptr<ClientBuffer> buffer,
    double consumer_resource_utilization) {
  DCHECK(thread_checker_.CalledOnValidThread());
  buffer = nullptr;
  if (client_buffers_.find(buffer_id) != client_buffers_.end()) {
    GetVideoCaptureHost()->ReleaseBuffer(device_id_, buffer_id,
                                         consumer_resource_utilization);
  }
}

// static
void VideoCaptureClient::DidFinishConsumingFrame(
    const media::VideoFrameMetadata* metadata,
    const BufferFinishedCallback& callback_to_io_thread) {
  // Note: This function may be called on any thread by the VideoFrame
  // destructor.  |metadata| is still valid for read-access at this point.
  double consumer_resource_utilization = -1.0;
  if (!metadata->GetDouble(media::VideoFrameMetadata::RESOURCE_UTILIZATION,
                           &consumer_resource_utilization)) {
    consumer_resource_utilization = -1.0;
  }
  callback_to_io_thread.Run(consumer_resource_utilization);
}

}  // namespace media
