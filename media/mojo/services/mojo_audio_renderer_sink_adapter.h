// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_AUDIO_RENDERER_SINK_ADAPTER_H_
#define MEDIA_MOJO_SERVICES_MOJO_AUDIO_RENDERER_SINK_ADAPTER_H_

#include "media/base/audio_renderer_sink.h"
#include "media/mojo/interfaces/audio_renderer_sink.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class SingleThreadTaskRunner;

class MEDIA_MOJO_EXPORT MojoAudioRendererSinkAdapter
    : public media::AudioRendererSink,
      public mojom::AudioRendererSinkRenderCallback {
 public:
  MojoAudioRendererSinkAdapter(const std::string& audio_device_id);

  void SetClient(mojom::AudioRendererSinkPtr sink);

  // AudioRendererSink implementation.
  void Initialize(const media::AudioParameters& params,
                  media::AudioRendererSink::RenderCallback* callback) override;
  void Start() override;
  void Stop() override;
  void Pause() override;
  void Play() override;
  bool SetVolume(double volume) override;
  media::OutputDeviceInfo GetOutputDeviceInfo() override;
  bool IsOptimizedForHardwareParameters() override;
  bool CurrentThreadIsRenderingThread() override;

  // mojom::AudioRendererSinkRenderCallback implementation.
  void Render(base::TimeDelta delay,
              base::TimeTicks delay_timestamp,
              int prior_frames_skipped,
              int nb_frames,
              media::mojom::AudioRendererSinkRenderCallback::RenderCallback
                  callback) final;
  void OnRenderError() final;

 protected:
  ~MojoAudioRendererSinkAdapter() override;

 private:
  mojo::Binding<mojom::AudioRendererSinkRenderCallback> binding_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::string audio_device_id_;
  bool started_;
  media::AudioRendererSink::RenderCallback* render_callback_;
  mojom::AudioRendererSinkPtr sink_;
  std::unique_ptr<AudioBus> audio_bus_;
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_AUDIO_RENDERER_SINK_ADAPTER_H_
