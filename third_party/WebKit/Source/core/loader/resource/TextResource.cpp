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
      decoder_(TextResourceDecoder::Create(decoder_options)) {}

TextResource::~TextResource() {}

void TextResource::SetEncoding(const String& chs) {
  decoder_->SetEncoding(WTF::TextEncoding(chs),
                        TextResourceDecoder::kEncodingFromHTTPHeader);
}

WTF::TextEncoding TextResource::Encoding() const {
  return decoder_->Encoding();
}

void TextResource::NotifyFinished() {
  if (Data()) {
    decoded_text_ = DecodedText();
    SetDecodedSize(decoded_text_.CharactersSizeInBytes());
  }

  Resource::NotifyFinished();

  // Clear raw bytes as now we have the full decoded sheet text.
  // We wait for all NotifyFinished() calls to run (at least once)
  // as SubresourceIntegrity checks require raw bytes.
  // Note that LinkStyle::NotifyFinished can be called from DidAddClient too,
  // but is safe as we should have a cached ResourceIntegrityDisposition.
  ClearData();
}

void TextResource::DestroyDecodedDataForFailedRevalidation() {
  decoded_text_ = String();
  SetDecodedSize(0);
}

String TextResource::DecodedText() const {
  if (!decoded_text_.IsNull())
    return decoded_text_;
  if (!Data() || Data()->IsEmpty())
    return String();
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
