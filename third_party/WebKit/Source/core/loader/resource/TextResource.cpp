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
  // The encoding may change depending on the resource's contents, so if we
  // haven't decoded the resource yet, we need to do so now.
  //
  // (Usually, Encoding() is only called after CachedDecodedText(), but some
  // test cases set up resources so that Encoding() is called before any other
  // decoding attempt. This claus catches those cases.)
  if (decoded_text_cache_.IsNull() && Data())
    CurrentDecodedText();

  return decoder_->Encoding();
}

String TextResource::CurrentDecodedText() const {
  DCHECK(Data());

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

const String& TextResource::CachedDecodedText() {
  DCHECK(IsLoaded());
  if (decoded_text_cache_.IsNull() && Data()) {
    decoded_text_cache_ = AtomicString(CurrentDecodedText());
    ClearData();
    SetDecodedSize(DecodedSize() + DecodedTextCacheSize());
  }
  return decoded_text_cache_;
}

const String& TextResource::CachedDecodedText() const {
  // TODO(vogelheim): CachedDecodedText() is logically a const operation, but
  //                  because we lazily update decoded_text_cache_ it
  //                  actually isn't. To avoid the const_cast<> below, we should
  //                  either make sure callees hold a non-const instance, or
  //                  make Resource::data_ mutable.
  return const_cast<TextResource*>(this)->CachedDecodedText();
}

void TextResource::ClearDecodedTextCache() {
  size_t old_size = DecodedTextCacheSize();
  decoded_text_cache_ = AtomicString();
  SetDecodedSize(DecodedSize() - old_size);
}

size_t TextResource::DecodedTextCacheSize() const {
  return decoded_text_cache_.CharactersSizeInBytes();
}

}  // namespace blink
