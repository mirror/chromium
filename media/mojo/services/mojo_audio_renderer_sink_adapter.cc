// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_audio_renderer_sink_adapter.h"

#include "media/mojo/common/media_type_converters.h"

namespace media {

MojoAudioRendererSinkAdapter::MojoAudioRendererSinkAdapter(
    const std::string& audio_device_id)
    : binding_(this),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      audio_device_id_(audio_device_id),
      started_(false),
      render_callback_(nullptr) {}

MojoAudioRendererSinkAdapter::~MojoAudioRendererSinkAdapter() {}

void MojoAudioRendererSinkAdapter::SetClient(mojom::AudioRendererSinkPtr sink) {
  CHECK(task_runner_->BelongsToCurrentThread());
  CHECK(sink);
  CHECK(!sink_);
  sink_ = std::move(sink);
}

void MojoAudioRendererSinkAdapter::Initialize(
    const media::AudioParameters& params,
    media::AudioRendererSink::RenderCallback* callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  CHECK(sink_);
  CHECK(!started_);
  audio_bus_ = AudioBus::Create(params);
  render_callback_ = callback;

  mojom::AudioRendererSinkRenderCallbackPtr render_callback_ptr;
  if (!binding_.is_bound())
    binding_.Bind(MakeRequest(&render_callback_ptr));

  sink_->Initialize(params, std::move(render_callback_ptr));
}

void MojoAudioRendererSinkAdapter::Start() {
  CHECK(!started_);
  started_ = true;
  sink_->Start();
}

void MojoAudioRendererSinkAdapter::Stop() {
  if (!started_)
    return;

  started_ = false;
  sink_->Stop();
}

void MojoAudioRendererSinkAdapter::Pause() {
  CHECK(sink_);
  sink_->Pause();
}

void MojoAudioRendererSinkAdapter::Play() {
  CHECK(sink_);
  sink_->Play();
}

bool MojoAudioRendererSinkAdapter::SetVolume(double volume) {
  bool success = false;
  sink_->SetVolume(volume, &success);
  return success;
}

media::OutputDeviceInfo MojoAudioRendererSinkAdapter::GetOutputDeviceInfo() {
  media::OutputDeviceInfo info;
  sink_->GetOutputDeviceInfo(&info);
  return info;
}

bool MojoAudioRendererSinkAdapter::IsOptimizedForHardwareParameters() {
  return true;
}

bool MojoAudioRendererSinkAdapter::CurrentThreadIsRenderingThread() {
  bool is_rendering_thread = false;
  sink_->CurrentThreadIsRenderingThread(&is_rendering_thread);
  return is_rendering_thread;
}

void MojoAudioRendererSinkAdapter::Render(
    base::TimeDelta delay,
    base::TimeTicks delay_timestamp,
    int prior_frames_skipped,
    int nb_frames,
    media::mojom::AudioRendererSinkRenderCallback::RenderCallback
        mojo_callback) {
  CHECK(task_runner_->BelongsToCurrentThread());

  if (!started_) {
    std::move(mojo_callback).Run(0, nullptr);
    return;
  }

  // See AudioRendererMixer::AddMixerInput
  // Expected to be capable of handling arbitrary
  // buffer size requests, disabling FIFO.
  if (audio_bus_->frames() != nb_frames)
    audio_bus_ = AudioBus::Create(audio_bus_->channels(), nb_frames);

  int nb_filled_frames = render_callback_->Render(
      delay, delay_timestamp, prior_frames_skipped, audio_bus_.get());

  // TODO(j.isorce): consider using AudioInputSyncWriter once it is mojified.
  std::move(mojo_callback)
      .Run(nb_filled_frames, mojom::AudioBus::From(*audio_bus_.get()));
}

void MojoAudioRendererSinkAdapter::OnRenderError() {
  render_callback_->OnRenderError();
}

}  // namespace media
