// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_AUDIO_CAPTURE_CLIENT_IMPL_H_
#define MEDIA_MOJO_SERVICES_MIRROR_AUDIO_CAPTURE_CLIENT_IMPL_H_

#include <string>

#include "base/callback.h"
#include "base/time/time.h"
#include "media/audio/audio_input_ipc.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_capturer_source.h"
#include "media/mojo/interfaces/audio_input_stream.mojom.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class AudioCaptureCallback final : public AudioCapturerSource::CaptureCallback {
 public:
  using AudioDataCallback =
      base::Callback<void(const AudioBus& bus,
                          base::TimeTicks estimated_capture_time)>;
  AudioCaptureCallback(const AudioDataCallback& audio_data_cb,
                       base::Callback<void(const std::string&)> error_cb);

  ~AudioCaptureCallback() override;

  // AudioCaptureSource::CaptureCallback implementations.
  void OnCaptureStarted() override;
  void Capture(const AudioBus* audio_source,
               int audio_delay_milliseconds,
               double volume,
               bool key_pressed) override;
  void OnCaptureError(const std::string& message) override;
  void OnCaptureMuted(bool is_muted) override;

 private:
  AudioDataCallback audio_data_cb_;
  base::Callback<void(const std::string&)> error_cb_;

  DISALLOW_COPY_AND_ASSIGN(AudioCaptureCallback);
};

class AudioCaptureIpc final : public AudioInputIPC,
                              public mojom::AudioInputStreamClient {
 public:
  explicit AudioCaptureIpc(mojom::AudioCaptureHostPtr host);

  ~AudioCaptureIpc() override;

  // AudioInputIPC implementations.
  void CreateStream(AudioInputIPCDelegate* delegate,
                    int session_id,
                    const AudioParameters& params,
                    bool automatic_gain_control,
                    uint32_t total_segments) override;
  void RecordStream() override;
  void SetVolume(double volume) override;
  void CloseStream() override;

  void OnStreamCreated(mojo::ScopedSharedBufferHandle handle,
                       mojo::ScopedHandle socket_mojo_handle,
                       bool initial_muted);

  // mojom::AudioInputStreamClient implementations.
  void OnError() override;
  void OnMutedStateChanged(bool is_muted) override;

 private:
  mojom::AudioCaptureHost* GetAudioCaptureHost();

  mojom::AudioCaptureHostPtrInfo audio_capture_host_info_;
  mojom::AudioCaptureHostPtr audio_capture_host_;
  mojom::AudioInputStreamPtr stream_;
  mojo::Binding<mojom::AudioInputStreamClient> binding_;
  AudioInputIPCDelegate* delegate_ = nullptr;
  base::WeakPtrFactory<AudioCaptureIpc> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioCaptureIpc);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_AUDIO_CAPTURE_CLIENT_IMPL_H_
