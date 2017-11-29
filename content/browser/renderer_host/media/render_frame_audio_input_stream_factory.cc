// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/render_frame_audio_input_stream_factory.h"

#include <utility>

#include "base/task_runner_util.h"
#include "content/browser/media/media_internals.h"
#include "media/base/audio_parameters.h"

namespace content {

// static
std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                BrowserThread::DeleteOnIOThread>
RenderFrameAudioInputStreamFactoryHandle::CreateFactory(
    RenderFrameAudioInputStreamFactory::CreateDelegateCallback
        create_delegate_callback,
    content::AudioInputDeviceManager* audio_input_device_manager,
    mojom::RendererAudioInputStreamFactoryRequest request) {
  std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                  BrowserThread::DeleteOnIOThread>
      handle(new RenderFrameAudioInputStreamFactoryHandle(
          std::move(create_delegate_callback), audio_input_device_manager));
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
        content::AudioInputDeviceManager* audio_input_device_manager)
    : impl_(std::move(create_delegate_callback), audio_input_device_manager),
      binding_(&impl_) {}

void RenderFrameAudioInputStreamFactoryHandle::Init(
    mojom::RendererAudioInputStreamFactoryRequest request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  binding_.Bind(std::move(request));
}

RenderFrameAudioInputStreamFactory::RenderFrameAudioInputStreamFactory(
    CreateDelegateCallback create_delegate_callback,
    content::AudioInputDeviceManager* audio_input_device_manager)
    : create_delegate_callback_(std::move(create_delegate_callback)),
#if defined(OS_CHROMEOS)
      audio_input_device_manager_(audio_input_device_manager),
#endif
      audio_log_(MediaInternals::GetInstance()->CreateAudioLog(
          media::AudioLogFactory::AUDIO_INPUT_CONTROLLER)),
      weak_ptr_factory_(this) {
  DCHECK(create_delegate_callback_);
  // No thread-hostile state has been initialized yet, so we don't have to bind
  // to this specific thread.
}

RenderFrameAudioInputStreamFactory::~RenderFrameAudioInputStreamFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void RenderFrameAudioInputStreamFactory::CreateStream(
    mojom::RendererAudioInputStreamFactoryClientPtr client,
    int64_t session_id,
    const media::AudioParameters& audio_params,
    bool automatic_gain_control,
    uint32_t shared_memory_count) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!base::IsValueInRangeForNumericType<int>(session_id)) {
    mojo::ReportBadMessage("session_id is not in integer range");
    return;
  }

#if defined(OS_CHROMEOS)
  if (audio_params.channel_layout() ==
      media::CHANNEL_LAYOUT_STEREO_AND_KEYBOARD_MIC) {
    audio_input_device_manager_->RegisterKeyboardMicStream(base::BindOnce(
        &RenderFrameAudioInputStreamFactory::DoCreateStream,
        weak_ptr_factory_.GetWeakPtr(), std::move(client), session_id,
        audio_params, automatic_gain_control, shared_memory_count));
    return;
  }
#endif
  DoCreateStream(std::move(client), session_id, audio_params,
                 automatic_gain_control, shared_memory_count,
                 AudioInputDeviceManager::KeyboardMicRegistration());
}

void RenderFrameAudioInputStreamFactory::DoCreateStream(
    mojom::RendererAudioInputStreamFactoryClientPtr client,
    int64_t session_id,
    const media::AudioParameters& audio_params,
    bool automatic_gain_control,
    uint32_t shared_memory_count,
    AudioInputDeviceManager::KeyboardMicRegistration
        keyboard_mic_registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int stream_id = ++next_stream_id_;

  streams_.insert(std::make_unique<AudioInputStreamHandle>(
      std::move(client),
      base::BindOnce(create_delegate_callback_, audio_log_.get(),
                     std::move(keyboard_mic_registration), shared_memory_count,
                     stream_id, session_id, automatic_gain_control,
                     audio_params),
      base::BindOnce(&RenderFrameAudioInputStreamFactory::RemoveStream,
                     weak_ptr_factory_.GetWeakPtr())));
}

void RenderFrameAudioInputStreamFactory::RemoveStream(
    AudioInputStreamHandle* stream) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::EraseIf(streams_,
                [stream](const std::unique_ptr<AudioInputStreamHandle>& other) {
                  return other.get() == stream;
                });
}

}  // namespace content
