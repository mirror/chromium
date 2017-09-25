// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webmediasource_impl.h"

#include "base/guid.h"
#include "media/base/mime_util.h"
#include "media/base/source_buffer.h"
#include "media/blink/websourcebuffer_impl.h"
#include "third_party/WebKit/public/platform/WebString.h"

using ::blink::WebString;
using ::blink::WebMediaSource;

namespace media {

#define STATIC_ASSERT_MATCHING_STATUS_ENUM(webkit_name, chromium_name) \
  static_assert(static_cast<int>(WebMediaSource::webkit_name) ==       \
                    static_cast<int>(SourceBuffer::chromium_name),     \
                "mismatching status enum values: " #webkit_name)
STATIC_ASSERT_MATCHING_STATUS_ENUM(kAddStatusOk, kOk);
STATIC_ASSERT_MATCHING_STATUS_ENUM(kAddStatusNotSupported, kNotSupported);
STATIC_ASSERT_MATCHING_STATUS_ENUM(kAddStatusReachedIdLimit, kReachedIdLimit);
#undef STATIC_ASSERT_MATCHING_STATUS_ENUM

WebMediaSourceImpl::WebMediaSourceImpl(SourceBuffer* source_buffer)
    : source_buffer_(source_buffer) {
  DCHECK(source_buffer_);
}

WebMediaSourceImpl::~WebMediaSourceImpl() {}

WebMediaSource::AddStatus WebMediaSourceImpl::AddSourceBuffer(
    const blink::WebString& type,
    const blink::WebString& codecs,
    blink::WebSourceBuffer** source_buffer) {
  std::string id = base::GenerateGUID();

  WebMediaSource::AddStatus result = static_cast<WebMediaSource::AddStatus>(
      source_buffer_->AddId(id, type.Utf8().data(), codecs.Utf8().data()));

  if (result == WebMediaSource::kAddStatusOk)
    *source_buffer = new WebSourceBufferImpl(id, source_buffer_);

  return result;
}

double WebMediaSourceImpl::Duration() {
  return source_buffer_->GetDuration();
}

void WebMediaSourceImpl::SetDuration(double new_duration) {
  DCHECK_GE(new_duration, 0);
  source_buffer_->SetDuration(new_duration);
}

void WebMediaSourceImpl::MarkEndOfStream(
    WebMediaSource::EndOfStreamStatus status) {
  PipelineStatus pipeline_status = PIPELINE_OK;

  switch (status) {
    case WebMediaSource::kEndOfStreamStatusNoError:
      break;
    case WebMediaSource::kEndOfStreamStatusNetworkError:
      pipeline_status = CHUNK_DEMUXER_ERROR_EOS_STATUS_NETWORK_ERROR;
      break;
    case WebMediaSource::kEndOfStreamStatusDecodeError:
      pipeline_status = CHUNK_DEMUXER_ERROR_EOS_STATUS_DECODE_ERROR;
      break;
  }

  source_buffer_->MarkEndOfStream(pipeline_status);
}

void WebMediaSourceImpl::UnmarkEndOfStream() {
  source_buffer_->UnmarkEndOfStream();
}

}  // namespace media
