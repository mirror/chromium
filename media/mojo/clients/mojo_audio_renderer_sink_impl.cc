// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_audio_renderer_sink_impl.h"

#include "media/mojo/common/media_type_converters.h"

namespace media {

MojoAudioRendererSinkImpl::MojoAudioRendererSinkImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    media::AudioRendererSink* sink,
    mojo::InterfaceRequest<mojom::AudioRendererSink> request)
    : media_task_runner_(media_task_runner),
      audio_device_task_runner_(nullptr),
      binding_(this, std::move(request)),
      sink_(sink),
      started_(false) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
}

MojoAudioRendererSinkImpl::~MojoAudioRendererSinkImpl() {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  if (started_) {
    sink_->Stop();
    started_ = false;
  }
}

int MojoAudioRendererSinkImpl::Render(base::TimeDelta delay,
                                      base::TimeTicks delay_timestamp,
                                      int prior_frames_skipped,
                                      AudioBus* dest) {
  if (!callback_.is_bound()) {
    callback_.Bind(std::move(callback_info_));

    if (!audio_device_task_runner_) {
      audio_device_task_runner_ = base::ThreadTaskRunnerHandle::Get();
      CHECK(audio_device_task_runner_ != media_task_runner_);
    }
  }

  CHECK(audio_device_task_runner_->BelongsToCurrentThread());

  int nb_filled_frames = 0;
  mojom::AudioBusPtr mojo_audio_bus;

  // Alternative solution is to bind mojom::AudioRendererSinkRenderCallbackPtr
  // to the audio device thread. Like
  callback_->Render(delay, delay_timestamp, prior_frames_skipped,
                    dest->frames(), &nb_filled_frames, &mojo_audio_bus);

  callback_info_ = callback_.PassInterface();

  if (!mojo_audio_bus)
    return 0;

  // TODO(j.isorce): consider using AudioSyncReader once it is mojified.
  std::unique_ptr<media::AudioBus> audio_bus_wrapper =
      media::AudioBus::WrapMemory(mojo_audio_bus->nb_channels,
                                  mojo_audio_bus->nb_frames,
                                  mojo_audio_bus->data.data());
  audio_bus_wrapper->CopyTo(dest);

  return nb_filled_frames;
}

void MojoAudioRendererSinkImpl::OnRenderError() {
  CHECK(audio_device_task_runner_->BelongsToCurrentThread());

  if (!callback_.is_bound())
    callback_.Bind(std::move(callback_info_));

  callback_->OnRenderError();

  callback_info_ = callback_.PassInterface();
}

void MojoAudioRendererSinkImpl::Initialize(
    const media::AudioParameters& params,
    mojom::AudioRendererSinkRenderCallbackPtr callback) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  if (callback)
    callback_info_ = callback.PassInterface();
  sink_->Initialize(params, this);
}

void MojoAudioRendererSinkImpl::Start() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  CHECK(!started_);

  started_ = true;
  sink_->Start();
}

void MojoAudioRendererSinkImpl::Stop() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  CHECK(started_);

  started_ = false;
  sink_->Stop();
}

void MojoAudioRendererSinkImpl::Pause() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  sink_->Pause();
}

void MojoAudioRendererSinkImpl::Play() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  sink_->Play();
}

void MojoAudioRendererSinkImpl::SetVolume(double volume,
                                          SetVolumeCallback callback) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  bool success = sink_->SetVolume(volume);
  std::move(callback).Run(success);
}

void MojoAudioRendererSinkImpl::GetOutputDeviceInfo(
    GetOutputDeviceInfoCallback callback) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  media::OutputDeviceInfo info = sink_->GetOutputDeviceInfo();
  std::move(callback).Run(info);
}

void MojoAudioRendererSinkImpl::CurrentThreadIsRenderingThread(
    CurrentThreadIsRenderingThreadCallback callback) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  bool is_rendering_thread = sink_->CurrentThreadIsRenderingThread();
  std::move(callback).Run(is_rendering_thread);
}

}  // namespace media
