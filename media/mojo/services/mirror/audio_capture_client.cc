// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/audio_capture_client.h"

#include "media/audio/audio_input_delegate.h"
#include "media/base/bind_to_current_loop.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

AudioCaptureCallback::AudioCaptureCallback(
    const AudioDataCallback& audio_data_cb,
    base::Callback<void(const std::string&)> error_cb)
    : audio_data_cb_(audio_data_cb), error_cb_(error_cb) {}

AudioCaptureCallback::~AudioCaptureCallback() {}

void AudioCaptureCallback::OnCaptureStarted() {
  DVLOG(1) << "Audio Capture started.";
}

void AudioCaptureCallback::Capture(const AudioBus* audio_source,
                                   int audio_delay_milliseconds,
                                   double volume,
                                   bool key_pressed) {
  if (!audio_data_cb_.is_null()) {
    audio_data_cb_.Run(*audio_source, base::TimeTicks::Now() -
                                          base::TimeDelta::FromMilliseconds(
                                              audio_delay_milliseconds));
  }
}

void AudioCaptureCallback::OnCaptureError(const std::string& message) {
  DVLOG(1) << message;
  if (!error_cb_.is_null())
    error_cb_.Run(message);
}

void AudioCaptureCallback::OnCaptureMuted(bool is_muted) {
  DVLOG(1) << __func__;
}

AudioCaptureIpc::AudioCaptureIpc(mojom::AudioCaptureHostPtr host)
    : binding_(this), weak_factory_(this) {
  audio_capture_host_info_ = host.PassInterface();
}

AudioCaptureIpc::~AudioCaptureIpc() {}

void AudioCaptureIpc::CreateStream(AudioInputIPCDelegate* delegate,
                                   int session_id,
                                   const AudioParameters& params,
                                   bool automatic_gain_control,
                                   uint32_t total_segments) {
  DCHECK(!delegate_);
  delegate_ = delegate;
  mojom::AudioInputStreamClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  GetAudioCaptureHost()->CreateStream(
      std::move(client), mojo::MakeRequest(&stream_),
      base::BindOnce(&AudioCaptureIpc::OnStreamCreated,
                     weak_factory_.GetWeakPtr()));
}

void AudioCaptureIpc::RecordStream() {
  if (!delegate_)
    return;
  DVLOG(1) << __func__;
  DCHECK(stream_);
  stream_->Record();
}

void AudioCaptureIpc::SetVolume(double volume) {
  DVLOG(1) << "Warning: " << __func__;
  DCHECK(stream_);
  stream_->SetVolume(volume);
}

void AudioCaptureIpc::CloseStream() {
  if (!delegate_)
    return;
  DVLOG(1) << __func__;
  GetAudioCaptureHost()->Stop();
  stream_.reset();
  delegate_ = nullptr;
}

void AudioCaptureIpc::OnStreamCreated(mojo::ScopedSharedBufferHandle handle,
                                      mojo::ScopedHandle socket_mojo_handle,
                                      bool initial_muted) {
  DVLOG(1) << __func__;

  base::SharedMemoryHandle memory_handle;
  size_t memory_size = 0;
  bool read_only_flag = false;

  DVLOG(1) << "UnwrapSharedMemoryHandle";
  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &memory_handle, &memory_size, &read_only_flag);
  DCHECK_EQ(MOJO_RESULT_OK, result);
  DCHECK_GT(memory_size, 0u);

  DVLOG(1) << "UnwrapPlatformFile";
  base::SyncSocket::Handle socket_handle;
  const MojoResult unwrap_socket_handle_result =
      mojo::UnwrapPlatformFile(std::move(socket_mojo_handle), &socket_handle);
  DCHECK_EQ(MOJO_RESULT_OK, unwrap_socket_handle_result);

  DCHECK(base::SharedMemory::IsHandleValid(memory_handle));
#if defined(OS_WIN)
  DCHECK(socket_handle);
#else
  DCHECK_GE(socket_handle, 0);
#endif
  DCHECK(delegate_);
  DVLOG(1) << "Notify delegate OnStreamCreated.";
  delegate_->OnStreamCreated(memory_handle, socket_handle, initial_muted);
}

void AudioCaptureIpc::OnError() {
  DVLOG(1) << __func__;
  if (!delegate_)
    return;
  delegate_->OnError();
}

void AudioCaptureIpc::OnMutedStateChanged(bool is_muted) {
  DVLOG(1) << __func__ << " is_muted: " << is_muted;
}

mojom::AudioCaptureHost* AudioCaptureIpc::GetAudioCaptureHost() {
  if (!audio_capture_host_.get())
    audio_capture_host_.Bind(std::move(audio_capture_host_info_));
  return audio_capture_host_.get();
}

}  // namespace media
