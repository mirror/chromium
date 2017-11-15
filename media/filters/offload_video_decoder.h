// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_OFFLOAD_VIDEO_DECODER_H_
#define MEDIA_FILTERS_OFFLOAD_VIDEO_DECODER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "media/base/demuxer_stream.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_frame_pool.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace media {

class MEDIA_EXPORT OffloadVideoDecoder : public VideoDecoder {
 public:
  using CreateDecoderCB =
      base::RepeatingCallback<std::unique_ptr<VideoDecoder>(void)>;

  OffloadVideoDecoder(std::vector<VideoCodec> supported_codecs,
                      CreateDecoderCB create_decoder_cb);
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
  std::vector<VideoCodec> supported_codecs_;

  // High resolution decodes may block the media thread for too long, in such
  // cases offload the decoding to a task pool.
  scoped_refptr<base::SequencedTaskRunner> offload_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(VpxVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_VPX_VIDEO_DECODER_H_
