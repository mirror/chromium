// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_OFFLOAD_VIDEO_DECODER_H_
#define MEDIA_FILTERS_OFFLOAD_VIDEO_DECODER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace media {

// Wrapper for VideoDecoder implementations that runs the wrapped decoder on a
// task pool other than the caller's thread.
//
// The only requirement for wrapped decoders is that after Reset() the decoder
// must allow Initialize() to be called on a different thread / sequence.
//
// Optionally decoders which are aware of the wrapping may choose to not rebind
// callbacks to the offloaded thread since they will already be bound by the
// OffloadVideoDecoder; this simply avoids extra hops for completed tasks.
class MEDIA_EXPORT OffloadVideoDecoder : public VideoDecoder {
 public:
  // Offloads |decoder| for VideoDecoderConfigs provided to Initialize() using
  // |supported_codecs| with a coded width > |offload_width|.
  OffloadVideoDecoder(int offload_width,
                      std::vector<VideoCodec> supported_codecs,
                      std::unique_ptr<VideoDecoder> decoder);
  ~OffloadVideoDecoder() override;

  // VideoDecoder implementation.
  std::string GetDisplayName() const override;
  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  CdmContext* cdm_context,
                  const InitCB& init_cb,
                  const OutputCB& output_cb) override;
  void Decode(const scoped_refptr<DecoderBuffer>& buffer,
              const DecodeCB& decode_cb) override;
  void Reset(const base::Closure& closure) override;

 private:
  // Video configurations over this width will be offloaded.
  const int offload_width_;

  // Codecs supported for offloading.
  const std::vector<VideoCodec> supported_codecs_;

  THREAD_CHECKER(thread_checker_);

  // The decoder which will be offloaded.
  std::unique_ptr<VideoDecoder> decoder_;

  // High resolution decodes may block the media thread for too long, in such
  // cases offload the decoding to a task pool.
  scoped_refptr<base::SequencedTaskRunner> offload_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(OffloadVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_OFFLOAD_VIDEO_DECODER_H_
