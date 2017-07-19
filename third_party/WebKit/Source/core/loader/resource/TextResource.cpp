// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/resource/TextResource.h"

#include "core/html/parser/TextResourceDecoder.h"
#include "platform/SharedBuffer.h"
#include "platform/loader/fetch/TextResourceDecoderOptions.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

TextResource::TextResource(const ResourceRequest& resource_request,
                           Resource::Type type,
                           const ResourceLoaderOptions& options,
                           const TextResourceDecoderOptions& decoder_options)
    : Resource(resource_request, type, options),
      decoder_(TextResourceDecoder::Create(decoder_options)),
      has_decoded_(false) {}

TextResource::~TextResource() {}

void TextResource::SetEncoding(const String& chs) {
  decoder_->SetEncoding(WTF::TextEncoding(chs),
                        TextResourceDecoder::kEncodingFromHTTPHeader);
  has_decoded_ = false;
}

WTF::TextEncoding TextResource::Encoding() const {
  // The decoder will may remember a character set declaration when decoding.
  // As a result, the encoding will not be correct before having ever used
  // the decoder.
  // So... if we haven't used decoder_ yet, we'll just force one decoder run.
  // In practice, this only seems to happen in tests, so it shouldn't be a
  // performance problem.
  if (!has_decoded_ && Data())
    DecodedText();

  return decoder_->Encoding();
}

String TextResource::DecodedText() const {
  DCHECK(Data());

  has_decoded_ = true;

  StringBuilder builder;
  const char* segment;
  size_t position = 0;
  while (size_t length = Data()->GetSomeData(segment, position)) {
    builder.Append(decoder_->Decode(segment, length));
    position += length;
  }
  builder.Append(decoder_->Flush());
  return builder.ToString();
}

}  // namespace blink
