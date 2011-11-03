// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_FFMPEG_VIDEO_DECODER_H_
#define MEDIA_FILTERS_FFMPEG_VIDEO_DECODER_H_

#include <deque>

#include "media/base/filters.h"
#include "media/base/pts_stream.h"
#include "media/video/video_decode_engine.h"
#include "ui/gfx/size.h"

class MessageLoop;

namespace media {

class VideoDecodeEngine;

class MEDIA_EXPORT FFmpegVideoDecoder
    : public VideoDecoder,
      public VideoDecodeEngine::EventHandler {
 public:
  explicit FFmpegVideoDecoder(MessageLoop* message_loop);
  virtual ~FFmpegVideoDecoder();

  // Filter implementation.
  virtual void Stop(const base::Closure& callback) OVERRIDE;
  virtual void Seek(base::TimeDelta time, const FilterStatusCB& cb) OVERRIDE;
  virtual void Pause(const base::Closure& callback) OVERRIDE;
  virtual void Flush(const base::Closure& callback) OVERRIDE;

  // VideoDecoder implementation.
  virtual void Initialize(DemuxerStream* demuxer_stream,
                          const base::Closure& callback,
                          const StatisticsCallback& stats_callback) OVERRIDE;
  virtual void ProduceVideoFrame(
      scoped_refptr<VideoFrame> video_frame) OVERRIDE;
  virtual gfx::Size natural_size() OVERRIDE;

 private:
  // VideoDecodeEngine::EventHandler interface.
  virtual void OnInitializeComplete(bool success) OVERRIDE;
  virtual void OnUninitializeComplete() OVERRIDE;
  virtual void OnFlushComplete() OVERRIDE;
  virtual void OnSeekComplete() OVERRIDE;
  virtual void OnError() OVERRIDE;
  virtual void ProduceVideoSample(scoped_refptr<Buffer> buffer) OVERRIDE;
  virtual void ConsumeVideoFrame(
      scoped_refptr<VideoFrame> frame,
      const PipelineStatistics& statistics) OVERRIDE;

  enum DecoderState {
    kUnInitialized,
    kInitializing,
    kNormal,
    kFlushCodec,
    kDecodeFinished,
    kPausing,
    kFlushing,
    kStopped
  };

  void OnFlushComplete(const base::Closure& callback);
  void OnSeekComplete(const base::Closure& callback);

  // TODO(scherkus): There are two of these to keep read completions
  // asynchronous and media_unittests passing. Remove.
  void OnReadComplete(const scoped_refptr<Buffer>& buffer);
  void OnReadCompleteTask(const scoped_refptr<Buffer>& buffer);

  // Flush the output buffers that we had held in Paused state.
  void FlushBuffers();

  // Injection point for unittest to provide a mock engine.  Takes ownership of
  // the provided engine.
  virtual void SetVideoDecodeEngineForTest(VideoDecodeEngine* engine);

  MessageLoop* message_loop_;

  PtsStream pts_stream_;  // Stream of presentation timestamps.
  DecoderState state_;
  scoped_ptr<VideoDecodeEngine> decode_engine_;

  base::Closure initialize_callback_;
  base::Closure uninitialize_callback_;
  base::Closure flush_callback_;
  FilterStatusCB seek_cb_;
  StatisticsCallback statistics_callback_;

  // Hold video frames when flush happens.
  std::deque<scoped_refptr<VideoFrame> > frame_queue_flushed_;

  // TODO(scherkus): I think this should be calculated by VideoRenderers based
  // on information provided by VideoDecoders (i.e., aspect ratio).
  gfx::Size natural_size_;

  // Pointer to the demuxer stream that will feed us compressed buffers.
  scoped_refptr<DemuxerStream> demuxer_stream_;

  DISALLOW_COPY_AND_ASSIGN(FFmpegVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_FFMPEG_VIDEO_DECODER_H_
