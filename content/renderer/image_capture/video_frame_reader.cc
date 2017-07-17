// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/image_capture/video_frame_reader.h"

#include "base/bind.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/video_frame.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/public/platform/WebMediaStreamSource.h"
#include "third_party/WebKit/public/platform/WebMediaStreamTrack.h"
#include "third_party/WebKit/public/platform/WebVideoFrameReaderClient.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"
namespace content {

VideoFrameReader::VideoFrameReader()
    : track_(nullptr), client_(nullptr), weak_factory_(this) {
  DVLOG(1) << __func__;
}

VideoFrameReader::~VideoFrameReader() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(1) << __func__;
  MediaStreamVideoSink::DisconnectFromTrack();
}

bool VideoFrameReader::Initialize(blink::WebMediaStreamTrack* track,
                                  blink::WebVideoFrameReaderClient* client) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(track && !track->IsNull() && track->GetTrackData());
  DCHECK_EQ(blink::WebMediaStreamSource::kTypeVideo, track->Source().GetType());

  client_ = client;
  track_ = track;

  // Frame delivery happens on a background thread, but we're not on it.
  MediaStreamVideoSink::ConnectToTrack(
      *track_,
      media::BindToCurrentLoop(base::Bind(&VideoFrameReader::OnVideoFrame,
                                          weak_factory_.GetWeakPtr())),
      false /* is_sink_secure */);

  return true;
}

void VideoFrameReader::OnVideoFrame(
    const scoped_refptr<media::VideoFrame>& frame,
    base::TimeTicks current_time) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(frame->format() == media::PIXEL_FORMAT_YV12 ||
         frame->format() == media::PIXEL_FORMAT_I420 ||
         frame->format() == media::PIXEL_FORMAT_YV12A);

  const SkAlphaType alpha = media::IsOpaque(frame->format())
                                ? kOpaque_SkAlphaType
                                : kPremul_SkAlphaType;
  const SkImageInfo info = SkImageInfo::MakeN32(
      frame->visible_rect().width(), frame->visible_rect().height(), alpha);

  sk_sp<SkSurface> surface = SkSurface::MakeRaster(info);
  DCHECK(surface);

  SkPixmap pixmap;
  if (!skia::GetWritablePixels(surface->getCanvas(), &pixmap)) {
    DLOG(ERROR) << "Error trying to map SkSurface's pixels";
    client_->OnError("Error trying to map SkSurface pixels");
  }

  const uint32 destination_pixel_format =
      (kN32_SkColorType == kRGBA_8888_SkColorType) ? libyuv::FOURCC_ABGR
                                                   : libyuv::FOURCC_ARGB;

  libyuv::ConvertFromI420(frame->visible_data(media::VideoFrame::kYPlane),
                          frame->stride(media::VideoFrame::kYPlane),
                          frame->visible_data(media::VideoFrame::kUPlane),
                          frame->stride(media::VideoFrame::kUPlane),
                          frame->visible_data(media::VideoFrame::kVPlane),
                          frame->stride(media::VideoFrame::kVPlane),
                          static_cast<uint8*>(pixmap.writable_addr()),
                          pixmap.width() * 4, pixmap.width(), pixmap.height(),
                          destination_pixel_format);

  if (frame->format() == media::PIXEL_FORMAT_YV12A) {
    DCHECK(!info.isOpaque());
    // This function copies any plane into the alpha channel of an ARGB image.
    libyuv::ARGBCopyYToAlpha(frame->visible_data(media::VideoFrame::kAPlane),
                             frame->stride(media::VideoFrame::kAPlane),
                             static_cast<uint8*>(pixmap.writable_addr()),
                             pixmap.width() * 4, pixmap.width(),
                             pixmap.height());
  }

  sk_sp<SkImage> image = surface->makeImageSnapshot();
  client_->OnVideoFrame(image);
}

}  // namespace content
