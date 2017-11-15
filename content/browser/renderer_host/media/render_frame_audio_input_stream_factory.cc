// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/render_frame_audio_input_stream_factory.h"

#include <utility>

#include "base/task_runner_util.h"
#include "media/base/audio_parameters.h"
#include "media/mojo/services/mojo_audio_input_stream.h"

namespace content {

// static
std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                BrowserThread::DeleteOnIOThread>
RenderFrameAudioInputStreamFactoryHandle::CreateFactory(
    RenderFrameAudioInputStreamFactory::CreateDelegateCallback
        create_delegate_callback,
    content::AudioInputDeviceManager* audio_input_device_manager,
    int render_process_id,
    int render_frame_id,
    mojom::RendererAudioInputStreamFactoryRequest request) {
  std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                  BrowserThread::DeleteOnIOThread>
      handle(new RenderFrameAudioInputStreamFactoryHandle(
          std::move(create_delegate_callback), audio_input_device_manager,
          render_process_id, render_frame_id));
  // Unretained is safe since |*handle| must be posted to the IO thread prior to
  // deletion.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&RenderFrameAudioInputStreamFactoryHandle::Init,
                     base::Unretained(handle.get()), std::move(request)));
  return handle;
}

RenderFrameAudioInputStreamFactoryHandle::
    ~RenderFrameAudioInputStreamFactoryHandle() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

RenderFrameAudioInputStreamFactoryHandle::
    RenderFrameAudioInputStreamFactoryHandle(
        RenderFrameAudioInputStreamFactory::CreateDelegateCallback
            create_delegate_callback,
        content::AudioInputDeviceManager* audio_input_device_manager,
        int render_process_id,
        int render_frame_id)
    : impl_(std::move(create_delegate_callback),
            audio_input_device_manager,
            render_process_id,
            render_frame_id),
      binding_(&impl_) {}

void RenderFrameAudioInputStreamFactoryHandle::Init(
    mojom::RendererAudioInputStreamFactoryRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  binding_.Bind(std::move(request));
}

RenderFrameAudioInputStreamFactory::RenderFrameAudioInputStreamFactory(
    CreateDelegateCallback create_delegate_callback,
    content::AudioInputDeviceManager* audio_input_device_manager,
    int render_process_id,
    int render_frame_id)
    : create_delegate_callback_(std::move(create_delegate_callback)),
#if defined(OS_CHROMEOS)
      audio_input_device_manager_(audio_input_device_manager),
#endif
      render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      weak_ptr_factory_(this) {
  DCHECK(create_delegate_callback_);
  // No thread-hostile state has been initialized yet, so we don't have to bind
  // to this specific thread.
}

RenderFrameAudioInputStreamFactory::~RenderFrameAudioInputStreamFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Make sure to close all streams.
  streams_.clear();
}

void RenderFrameAudioInputStreamFactory::CreateStream(
    media::mojom::AudioInputStreamRequest stream_request,
    int64_t session_id,
    const media::AudioParameters& audio_params,
    bool automatic_gain_control,
    uint32_t shared_memory_count,
    media::mojom::AudioInputStreamClientPtr client,
    CreateStreamCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
#if defined(OS_CHROMEOS)
  if (audio_params.channel_layout() ==
      media::CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC) {
    audio_input_device_manager_->RegisterKeyboardMicStream(base::BindOnce(
        &RenderFrameAudioInputStreamFactory::DoCreateStream,
        weak_ptr_factory_.GetWeakPtr(), std::move(stream_request), session_id,
        audio_params, automatic_gain_control, shared_memory_count,
        std::move(client), std::move(callback)));
    return;
  }
#endif
  DoCreateStream(std::move(stream_request), session_id, audio_params,
                 automatic_gain_control, shared_memory_count, std::move(client),
                 std::move(callback),
                 AudioInputDeviceManager::KeyboardMicRegistration());
}

void RenderFrameAudioInputStreamFactory::DoCreateStream(
    media::mojom::AudioInputStreamRequest stream_request,
    int64_t session_id,
    const media::AudioParameters& audio_params,
    bool automatic_gain_control,
    uint32_t shared_memory_count,
    media::mojom::AudioInputStreamClientPtr client,
    CreateStreamCallback callback,
    AudioInputDeviceManager::KeyboardMicRegistration
        keyboard_mic_registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int stream_id = ++next_stream_id_;
  streams_.insert(std::make_unique<media::MojoAudioInputStream>(
      std::move(stream_request), std::move(client),
      base::BindOnce(create_delegate_callback_,
                     std::move(keyboard_mic_registration), shared_memory_count,
                     stream_id, session_id, render_process_id_,
                     render_frame_id_, automatic_gain_control, audio_params),
      std::move(callback),
      base::BindOnce(&RenderFrameAudioInputStreamFactory::RemoveStream,
                     weak_ptr_factory_.GetWeakPtr())));
}

void RenderFrameAudioInputStreamFactory::RemoveStream(
    media::mojom::AudioInputStream* stream) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::EraseIf(
      streams_,
      [stream](const std::unique_ptr<media::mojom::AudioInputStream>& other) {
        return other.get() == stream;
      });
}

}  // namespace content
