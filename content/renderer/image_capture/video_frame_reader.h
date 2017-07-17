// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IMAGE_CAPTURE_VIDEO_FRAME_READER_H_
#define CONTENT_RENDERER_IMAGE_CAPTURE_VIDEO_FRAME_READER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "content/common/content_export.h"
#include "content/public/renderer/media_stream_video_sink.h"
#include "third_party/WebKit/public/platform/WebVideoFrameReader.h"

namespace blink {
class WebMediaStreamTrack;
class WebVideoFrameReaderClient;
}  // namespace blink

namespace content {

class CONTENT_EXPORT VideoFrameReader final
    : NON_EXPORTED_BASE(public blink::WebVideoFrameReader),
      NON_EXPORTED_BASE(public MediaStreamVideoSink) {
 public:
  VideoFrameReader();
  ~VideoFrameReader() override;

  // blink::WebImageCaptureFrameGrabber implementation.
  bool Initialize(blink::WebMediaStreamTrack* track,
                  blink::WebVideoFrameReaderClient* client) override;

  // Local VideoCaptureDeliverFrameCB.
  void OnVideoFrame(const scoped_refptr<media::VideoFrame>& frame,
                    base::TimeTicks current_time);

 private:
  // We need to hold on to the Blink track to remove ourselves on dtor.
  blink::WebMediaStreamTrack* track_;

  // |client_| is a weak pointer, and is valid for the lifetime of this object.
  blink::WebVideoFrameReaderClient* client_;

  // Used to check that we are destroyed on the same thread we were created.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<VideoFrameReader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameReader);
};

}  // namespace content

#endif  // CONTENT_RENDERER_IMAGE_CAPTURE_VIDEO_FRAME_READER_H_
