// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_WRAPPER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_WRAPPER_H_

#include "base/macros.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace chromecast {
namespace media {

class MediaPipelineBackendManager;

class AudioDecoderWrapper : public MediaPipelineBackend::AudioDecoder {
 public:
  AudioDecoderWrapper(MediaPipelineBackendManager* backend_manager,
                      MediaPipelineBackend::AudioDecoder* decoder);
  ~AudioDecoderWrapper() override;

  void SetGlobalVolumeMultiplier(float multiplier);

 private:
  // MediaPipelineBackend::AudioDecoder implementation:
  void SetDelegate(Delegate* delegate) override;
  BufferStatus PushBuffer(CastDecoderBuffer* buffer) override;
  bool SetConfig(const AudioConfig& config) override;
  bool SetVolume(float multiplier) override;
  RenderingDelay GetRenderingDelay() override;
  void GetStatistics(Statistics* statistics) override;

  MediaPipelineBackendManager* const backend_manager_;
  MediaPipelineBackend::AudioDecoder* const decoder_;

  DISALLOW_COPY_AND_ASSIGN(MediaPipelineBackendWrapper);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_WRAPPER_H_
