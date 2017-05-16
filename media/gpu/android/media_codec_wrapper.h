// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_MEDIA_CODEC_WRAPPER_
#define MEDIA_GPU_ANDROID_MEDIA_CODEC_WRAPPER_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/base/video_decoder_config.h"
#include "media/gpu/avda_surface_bundle.h"

namespace media {

// This wraps a MediaCodecBridge. It adds three things:
// * Tracks some codec specific state that in AVDA we maintained separately
//   (e.g., config(), IsDrained(), NumBuffersInCodec()). This makes it easy to
//   know for example which surface is attached. In AVDA this is relatively hard
//   and error prone.
// * Makes MediaCodec threadsafe. We need to call ReleaseOutputBuffer() when
//   the corresponding textures are drawn on the GPU thread while the MCVD
//   thread is potentially also accessing it.
//
// * (Potentially) Outlives the codec. i.e., Release() will release the delegate
//   codec and the released state can be queried. This, along with making it
//   refcounted, would let us easily share the codec with Images and be free to
//   delete the codec without synchronizing with the images; they can check
//   whether the codec is gone or not. Basically it's a way to emulate a
//   threadsafe weak pointer.
class MediaCodecWrapper : public MediaCodecBridge {
 public:
  explicit MediaCodecWrapper(std::unique_ptr<MediaCodecBridge> codec);
  ~MediaCodecWrapper() override;

  bool NeedsDrainingBeforeRelease();
  bool IsEmpty();
  bool EosSubmitted();
  bool IsFlushed();
  bool SupportsFlush();
  const VideoDecoderConfig& config() { return config_; }
  int surface_id() { return surface_id_; }
  void set_surface_bundle(scoped_refptr<AVDASurfaceBundle> bundle) {
    surface_bundle_ = bundle;
  }

  // MediaCodecBridge implementation:
  void Stop() override;
  MediaCodecStatus Flush() override;
  MediaCodecStatus GetOutputSize(gfx::Size* size) override;
  MediaCodecStatus GetOutputSamplingRate(int* sampling_rate) override;
  MediaCodecStatus GetOutputChannelCount(int* channel_count) override;
  MediaCodecStatus QueueInputBuffer(int index,
                                    const uint8_t* data,
                                    size_t data_size,
                                    base::TimeDelta presentation_time) override;
  MediaCodecStatus QueueSecureInputBuffer(
      int index,
      const uint8_t* data,
      size_t data_size,
      const std::string& key_id,
      const std::string& iv,
      const std::vector<SubsampleEntry>& subsamples,
      const EncryptionScheme& encryption_scheme,
      base::TimeDelta presentation_time) override;
  void QueueEOS(int input_buffer_index) override;
  MediaCodecStatus DequeueInputBuffer(base::TimeDelta timeout,
                                      int* index) override;
  MediaCodecStatus DequeueOutputBuffer(base::TimeDelta timeout,
                                       int* index,
                                       size_t* offset,
                                       size_t* size,
                                       base::TimeDelta* presentation_time,
                                       bool* end_of_stream,
                                       bool* key_frame) override;
  void ReleaseOutputBuffer(int index, bool render) override;
  MediaCodecStatus GetInputBuffer(int input_buffer_index,
                                  uint8_t** data,
                                  size_t* capacity) override;
  MediaCodecStatus CopyFromOutputBuffer(int index,
                                        size_t offset,
                                        void* dst,
                                        size_t num) override;
  std::string GetName() override;
  bool SetSurface(jobject surface) override;
  void SetVideoBitrate(int bps, int frame_rate) override;
  void RequestKeyFrameSoon() override;
  bool IsAdaptivePlaybackSupported() override;

 private:
  int num_buffers_in_codec_ = 0;
  VideoDecoderConfig config_;
  int surface_id_;
  scoped_refptr<AVDASurfaceBundle> surface_bundle_;

  std::unique_ptr<MediaCodecBridge> codec_;

  DISALLOW_COPY_AND_ASSIGN(MediaCodecWrapper);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_MEDIA_CODEC_WRAPPER_
