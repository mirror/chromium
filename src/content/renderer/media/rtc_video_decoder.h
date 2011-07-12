// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_RTC_VIDEO_DECODER_H_
#define CONTENT_RENDERER_MEDIA_RTC_VIDEO_DECODER_H_

#include <deque>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/time.h"
#include "media/base/filters.h"
#include "media/base/video_frame.h"
#include "media/filters/decoder_base.h"
#include "third_party/libjingle/source/talk/session/phone/mediachannel.h"

class RTCVideoDecoder
    : public media::VideoDecoder,
      public cricket::VideoRenderer {
 public:
  RTCVideoDecoder(MessageLoop* message_loop, const std::string& url);
  virtual ~RTCVideoDecoder();

  // Filter implementation.
  virtual void Play(media::FilterCallback* callback);
  virtual void Seek(base::TimeDelta time, const media::FilterStatusCB& cb);
  virtual void Pause(media::FilterCallback* callback);
  virtual void Stop(media::FilterCallback* callback);

  // Decoder implementation.
  virtual void Initialize(media::DemuxerStream* demuxer_stream,
                          media::FilterCallback* filter_callback,
                          media::StatisticsCallback* stat_callback);
  virtual const media::MediaFormat& media_format();
  virtual void ProduceVideoFrame(scoped_refptr<media::VideoFrame> video_frame);
  virtual bool ProvidesBuffer();

  // cricket::VideoRenderer implementation
  virtual bool SetSize(int width, int height, int reserved);
  virtual bool RenderFrame(const cricket::VideoFrame *frame);

 private:
  friend class RTCVideoDecoderTest;
  FRIEND_TEST_ALL_PREFIXES(RTCVideoDecoderTest, Initialize_Successful);
  FRIEND_TEST_ALL_PREFIXES(RTCVideoDecoderTest, DoSeek);
  FRIEND_TEST_ALL_PREFIXES(RTCVideoDecoderTest, DoRenderFrame);
  FRIEND_TEST_ALL_PREFIXES(RTCVideoDecoderTest, DoSetSize);

  enum DecoderState {
    kUnInitialized,
    kNormal,
    kSeeking,
    kPaused,
    kStopped
  };

  MessageLoop* message_loop_;
  size_t width_;
  size_t height_;
  std::string url_;
  DecoderState state_;
  media::MediaFormat media_format_;
  std::deque<scoped_refptr<media::VideoFrame> > frame_queue_available_;
  // Used for accessing frame queue from another thread.
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(RTCVideoDecoder);
};

#endif  // CONTENT_RENDERER_MEDIA_RTC_VIDEO_DECODER_H_
