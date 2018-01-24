// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIRROR_SERVICE_AUDIO_CAPTURE_HOST_H_
#define CHROME_BROWSER_MEDIA_MIRROR_SERVICE_AUDIO_CAPTURE_HOST_H_

#include <string>

#include "base/callback.h"
#include "content/public/common/media_stream_request.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_system.h"
#include "media/base/audio_parameters.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "media/mojo/services/mojo_audio_input_stream.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {
class AudioInputSyncWriter;
}

namespace media {

class AudioDelegate final : public AudioInputDelegate,
                            public AudioInputStream::AudioInputCallback {
 public:
  AudioDelegate(AudioInputDelegate::EventHandler* event_handler,
                scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);
  ~AudioDelegate() override;

  void CreateStream(const std::string& device_id,
                    const AudioParameters& params);
  void Stop();

  // AudioInputDelegate implemenations.
  void OnRecordStream() override;
  int GetStreamId() override;
  void OnSetVolume(double volume) override;

  // AudioInputCallback implementations.
  void OnData(const AudioBus* source,
              base::TimeTicks capture_time,
              double volume) override;

  void OnError() override;

 private:
  AudioInputDelegate::EventHandler* event_handler_;
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;
  std::unique_ptr<content::AudioInputSyncWriter> writer_;
  AudioInputStream* stream_ = nullptr;
  double max_volume_ = 0.0;
  base::WeakPtrFactory<AudioDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioDelegate);
};

class MirrorServiceAudioCaptureHost final : public mojom::AudioCaptureHost {
 public:
  MirrorServiceAudioCaptureHost(
      const std::string& device_id,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);

  ~MirrorServiceAudioCaptureHost() override;

  using AudioCaptureHostCreatedCallback =
      base::OnceCallback<void(const AudioParameters& params,
                              mojom::AudioCaptureHostPtr host)>;
  static void Create(
      const std::string& device_id,
      content::MediaStreamType type,
      AudioCaptureHostCreatedCallback callback,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);

  static void OnAudioParameters(
      std::unique_ptr<MirrorServiceAudioCaptureHost> host,
      AudioCaptureHostCreatedCallback callback,
      const base::Optional<AudioParameters>& opt_params);

  void GetAudioParameters(content::MediaStreamType type,
                          AudioSystem::OnAudioParamsCallback callback);

  void SetParams(const AudioParameters& params);

  // mojom::AudioCaptureHost implementations.
  void CreateStream(mojom::AudioInputStreamClientPtr client,
                    mojom::AudioInputStreamRequest stream,
                    MojoAudioInputStream::StreamCreatedCallback cb) override;
  void Stop() override;

  std::unique_ptr<AudioInputDelegate> GetAudioInputDelegate(
      AudioInputDelegate::EventHandler* event_handler);

 private:
  const std::string device_id_;
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;
  AudioParameters params_;
  std::unique_ptr<MojoAudioInputStream> stream_;
  std::unique_ptr<AudioSystem> audio_system_;
  AudioDelegate* delegate_ = nullptr;  // Own by |stream_|.
  base::WeakPtrFactory<MirrorServiceAudioCaptureHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MirrorServiceAudioCaptureHost);
};

}  // namespace media

#endif  // CHROME_BROWSER_MEDIA_MIRROR_SERVICE_AUDIO_CAPTURE_HOST_H_
