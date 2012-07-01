// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_FFMPEG_VIDEO_DECODER_H_
#define MEDIA_FILTERS_FFMPEG_VIDEO_DECODER_H_

#include <deque>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "media/base/video_decoder.h"

class MessageLoop;

struct AVCodecContext;
struct AVFrame;

namespace media {

class DecoderBuffer;
class Decryptor;

class MEDIA_EXPORT FFmpegVideoDecoder : public VideoDecoder {
 public:
  FFmpegVideoDecoder(const base::Callback<MessageLoop*()>& message_loop_cb);

  // VideoDecoder implementation.
  virtual void Initialize(const scoped_refptr<DemuxerStream>& stream,
                          const PipelineStatusCB& status_cb,
                          const StatisticsCB& statistics_cb) OVERRIDE;
  virtual void Read(const ReadCB& read_cb) OVERRIDE;
  virtual void Reset(const base::Closure& closure) OVERRIDE;
  virtual void Stop(const base::Closure& closure) OVERRIDE;
  virtual const gfx::Size& natural_size() OVERRIDE;

  // Must be called prior to initialization if decrypted buffers will be
  // encountered.
  void set_decryptor(Decryptor* decryptor);

  // Callback called from within FFmpeg to allocate a buffer based on
  // the dimensions of |codec_context|. See AVCodecContext.get_buffer
  // documentation inside FFmpeg.
  int GetVideoBuffer(AVCodecContext *codec_context, AVFrame* frame);

 protected:
  virtual ~FFmpegVideoDecoder();

 private:
  enum DecoderState {
    kUninitialized,
    kNormal,
    kFlushCodec,
    kDecodeFinished,
  };

  // Carries out the reading operation scheduled by Read().
  void DoRead(const ReadCB& read_cb);

  // Reads from the demuxer stream with corresponding callback method.
  void ReadFromDemuxerStream();
  void DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer);

  // Carries out the decoding operation scheduled by DecodeBuffer().
  void DoDecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer);
  bool Decode(const scoped_refptr<DecoderBuffer>& buffer,
              scoped_refptr<VideoFrame>* video_frame);

  // Delivers the frame to |read_cb_| and resets the callback.
  void DeliverFrame(const scoped_refptr<VideoFrame>& video_frame);

  // Releases resources associated with |codec_context_| and |av_frame_|
  // and resets them to NULL.
  void ReleaseFFmpegResources();

  // Reset decoder and call |reset_cb_|.
  void DoReset();

  // This is !is_null() iff Initialize() hasn't been called.
  base::Callback<MessageLoop*()> message_loop_factory_cb_;

  MessageLoop* message_loop_;

  DecoderState state_;

  StatisticsCB statistics_cb_;

  ReadCB read_cb_;
  base::Closure reset_cb_;

  // FFmpeg structures owned by this object.
  AVCodecContext* codec_context_;
  AVFrame* av_frame_;

  // Frame rate of the video.
  int frame_rate_numerator_;
  int frame_rate_denominator_;

  // TODO(scherkus): I think this should be calculated by VideoRenderers based
  // on information provided by VideoDecoders (i.e., aspect ratio).
  gfx::Size natural_size_;

  // Pointer to the demuxer stream that will feed us compressed buffers.
  scoped_refptr<DemuxerStream> demuxer_stream_;

  Decryptor* decryptor_;

  DISALLOW_COPY_AND_ASSIGN(FFmpegVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_FFMPEG_VIDEO_DECODER_H_
