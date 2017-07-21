// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/mojo_audio_input_ipc.h"

#include <utility>

#include "media/audio/audio_device_description.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace content {

MojoAudioInputIPC::MojoAudioInputIPC(FactoryAccessorCB factory_accessor)
    : factory_accessor_(factory_accessor), client_binding_(this) {}

MojoAudioInputIPC::~MojoAudioInputIPC() = default;

void MojoAudioInputIPC::CreateStream(media::AudioInputIPCDelegate* delegate,
                                     int session_id,
                                     const media::AudioParameters& params,
                                     bool automatic_gain_control,
                                     uint32_t total_segments) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(delegate);
  DCHECK(!delegate_);

  delegate_ = delegate;

  auto* factory = factory_accessor_.Run();
  if (!factory) {
    LOG(ERROR) << "MojoAudioInputIPC failed to acquire factory";
    // TODO(ossu): What to do here? OnError? Die in a fire of CHECKs()?
    return;
  }

  media::mojom::AudioInputStreamClientPtr client;
  client_binding_.Bind(mojo::MakeRequest(&client));

  factory->CreateStream(
      mojo::MakeRequest(&stream_), session_id, params, automatic_gain_control,
      total_segments, std::move(client),
      base::Bind(&MojoAudioInputIPC::StreamCreated, base::Unretained(this)));

  stream_.set_connection_error_handler(base::BindOnce(
      &media::AudioInputIPCDelegate::OnError, base::Unretained(delegate_)));
}

void MojoAudioInputIPC::RecordStream() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (stream_.is_bound())
    stream_->Record();
}

void MojoAudioInputIPC::SetVolume(double volume) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (stream_.is_bound())
    stream_->SetVolume(volume);
}

void MojoAudioInputIPC::CloseStream() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_binding_.Close();
  stream_.reset();
  delegate_ = nullptr;
}

void MojoAudioInputIPC::StreamCreated(
    mojo::ScopedSharedBufferHandle shared_memory,
    mojo::ScopedHandle socket,
    uint32_t length,
    uint32_t segment_count,
    bool initially_muted) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(delegate_);
  DCHECK(socket.is_valid());
  DCHECK(shared_memory.is_valid());

  base::PlatformFile socket_handle;
  auto result = mojo::UnwrapPlatformFile(std::move(socket), &socket_handle);
  DCHECK_EQ(result, MOJO_RESULT_OK);

  // TODO(ossu): What to do about length vs. memory_length?
  base::SharedMemoryHandle memory_handle;
  bool read_only = true;
  size_t memory_length = 0;
  result = mojo::UnwrapSharedMemoryHandle(
      std::move(shared_memory), &memory_handle, &memory_length, &read_only);
  DCHECK_EQ(result, MOJO_RESULT_OK);
  DCHECK(read_only);

  delegate_->OnStreamCreated(memory_handle, socket_handle, length,
                             segment_count, initially_muted);
}

void MojoAudioInputIPC::OnMuted(bool is_muted) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(delegate_);
  delegate_->OnMuted(is_muted);
}

}  // namespace content
