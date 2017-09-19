// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_AUDIO_RENDERER_SINK_IMPL_H_
#define MEDIA_MOJO_CLIENTS_MOJO_AUDIO_RENDERER_SINK_IMPL_H_

#include <memory>

#include "media/base/audio_renderer_sink.h"
#include "media/mojo/interfaces/audio_renderer_sink.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class MojoAudioRendererSinkImpl
    : public media::AudioRendererSink::RenderCallback,
      public mojom::AudioRendererSink {
 public:
  MojoAudioRendererSinkImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      media::AudioRendererSink* sink,
      mojo::InterfaceRequest<mojom::AudioRendererSink> request);
  ~MojoAudioRendererSinkImpl() override;

  // AudioRendererSink::RenderCallback implementation.
  int Render(base::TimeDelta delay,
             base::TimeTicks delay_timestamp,
             int prior_frames_skipped,
             AudioBus* dest) override;
  void OnRenderError() override;

  // mojom::AudioRendererSink implementation.
  void Initialize(
      const media::AudioParameters& params,
      mojom::AudioRendererSinkRenderCallbackPtr render_callback) final;
  void Start() final;
  void Stop() final;
  void Pause() final;
  void Play() final;
  void SetVolume(double volume, SetVolumeCallback callback) final;
  void GetOutputDeviceInfo(GetOutputDeviceInfoCallback callback) final;
  void CurrentThreadIsRenderingThread(
      CurrentThreadIsRenderingThreadCallback callback) final;

  // Sets an error handler that will be called if a connection error occurs on
  // the bound message pipe.
  void set_connection_error_handler(const base::Closure& error_handler) {
    binding_.set_connection_error_handler(error_handler);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> audio_device_task_runner_;

  mojo::Binding<mojom::AudioRendererSink> binding_;

  media::AudioRendererSink* sink_;
  bool started_;

  mojom::AudioRendererSinkRenderCallbackPtrInfo callback_info_;
  mojom::AudioRendererSinkRenderCallbackPtr callback_;
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_AUDIO_RENDERER_SINK_IMPL_H_
