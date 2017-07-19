// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/streams/MediaStreamTrackSource.h"

#include "bindings/core/v8/CallbackPromiseAdapter.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMTypedArray.h"
#include "core/html/ImageData.h"
#include "core/imagebitmap/ImageBitmap.h"
#include "core/streams/ReadableStreamController.h"
#include "core/streams/ReadableStreamOperations.h"
#include "modules/mediastream/MediaStreamTrack.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/V8PrivateProperty.h"
#include "platform/bindings/V8ThrowException.h"
#include "public/platform/Platform.h"
#include "public/platform/WebVideoFrameReader.h"

namespace blink {

namespace {

bool TrackIsInactive(const MediaStreamTrack& track) {
  return track.readyState() != "live" || !track.enabled();
}

}  // namespace

MediaStreamTrackSource::MediaStreamTrackSource(ScriptState* script_state,
                                               MediaStreamTrack& track)
    : UnderlyingSourceBase(script_state),
      script_state_(script_state),
      stream_track_(&track) {
  DVLOG(1) << __func__;
}

ScriptPromise MediaStreamTrackSource::Start(ScriptState* script_state) {
  DVLOG(1) << __func__;
  DCHECK_EQ(script_state, script_state_.Get());

  if (stream_track_->kind() != "video") {
    auto* resolver = ScriptPromiseResolver::Create(script_state);
    ScriptPromise promise = resolver->Promise();
    resolver->Reject(DOMException::Create(kInvalidStateError,
                                          "Only Video Tracks supported"));
    return promise;
  }

  if (TrackIsInactive(*stream_track_)) {
    auto* resolver = ScriptPromiseResolver::Create(script_state);
    ScriptPromise promise = resolver->Promise();
    resolver->Reject(DOMException::Create(
        kInvalidStateError, "The associated Track is in an invalid state."));
    return promise;
  }

  if (!video_frame_reader_)
    video_frame_reader_ = Platform::Current()->CreateVideoFrameReader();

  if (!video_frame_reader_) {
    auto* resolver = ScriptPromiseResolver::Create(script_state);
    ScriptPromise promise = resolver->Promise();
    resolver->Reject(DOMException::Create(
        kUnknownError, "Couldn't create platform resources"));
    return promise;
  }

  // The platform does not know about MediaStreamTrack, so we wrap it up.
  WebMediaStreamTrack track(stream_track_->Component());
  video_frame_reader_->Initialize(&track, this);

  // TODO(mcasas): Kick the first collection of data.

  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise MediaStreamTrackSource::pull(ScriptState* script_state) {
  DVLOG(1) << __func__;
  DCHECK_EQ(script_state, script_state_.Get());

  if (TrackIsInactive(*stream_track_)) {
    auto* resolver = ScriptPromiseResolver::Create(script_state);
    ScriptPromise promise = resolver->Promise();
    resolver->Reject(DOMException::Create(
        kInvalidStateError, "The associated Track is in an invalid state."));
    return promise;
  }

  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise MediaStreamTrackSource::Cancel(ScriptState* script_state,
                                             ScriptValue reason) {
  DCHECK_EQ(script_state, script_state_.Get());

  return ScriptPromise::CastUndefined(script_state);
}

bool MediaStreamTrackSource::HasPendingActivity() const {
  return UnderlyingSourceBase::HasPendingActivity();
}

void MediaStreamTrackSource::ContextDestroyed(
    ExecutionContext* destroyed_context) {
  DVLOG(1) << __func__;
  UnderlyingSourceBase::ContextDestroyed(destroyed_context);
  video_frame_reader_.reset();
}

void MediaStreamTrackSource::OnVideoFrame(sk_sp<SkImage> sk_image) {
  DVLOG(1) << __func__ << " " << sk_image->width() << "x" << sk_image->height();

  // If we can access the pixels then propagate |sk_image| as an ImageData,
  // otherwise as an ImageBitmap.

  DCHECK(!sk_image->isTextureBacked());
  SkPixmap pixmap;
  if (sk_image->peekPixels(&pixmap)) {
    DOMUint8ClampedArray* data_u8 = DOMUint8ClampedArray::Create(
        static_cast<uint8_t*>(pixmap.writable_addr()), pixmap.getSafeSize());

    DCHECK(data_u8);
    DVLOG(1) << __func__ << " " << data_u8->length() << "B";

    const IntSize size(sk_image->width(), sk_image->height());
    DOMArrayBufferView* data_array = static_cast<DOMArrayBufferView*>(data_u8);
    ImageDataColorSettings color;

    ImageData* image_data = ImageData::Create(
        size, NotShared<DOMArrayBufferView>(data_array), &color);

    Controller()->Enqueue(image_data);
    return;
  }

  auto bitmap_image = StaticBitmapImage::Create(std::move(sk_image));
  ImageBitmap* blink_image = ImageBitmap::Create(bitmap_image.Get());
  Controller()->Enqueue(blink_image);
}

void MediaStreamTrackSource::OnError(const WebString& message) {
  DLOG(ERROR) << message.Ascii();
}

DEFINE_TRACE(MediaStreamTrackSource) {
  visitor->Trace(stream_track_);
  UnderlyingSourceBase::Trace(visitor);
}

}  // namespace blink
