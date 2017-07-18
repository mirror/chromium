// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/resource/TextIntegrityResource.h"

#include "core/frame/SubresourceIntegrity.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/IntegrityMetadata.h"

namespace blink {

const String& TextIntegrityResource::SourceText() {
  DCHECK(IsLoaded());

  if (source_text_.IsNull() && Data()) {
    String source_text = DecodedText();
    ClearData();
    SetDecodedSize(source_text.CharactersSizeInBytes());
    source_text_ = AtomicString(source_text);
  }
  return source_text_;
}

void TextIntegrityResource::SetIntegrityMetadataFrom(
    const FetchParameters& params) {
  if (!params.IntegrityMetadata().IsEmpty())
    SetIntegrityMetadata(params.IntegrityMetadata());
}

void TextIntegrityResource::SetIntegrityDisposition(
    ResourceIntegrityDisposition disposition) {
  DCHECK_NE(disposition, ResourceIntegrityDisposition::kNotChecked);
  integrity_disposition_ = disposition;
}

void TextIntegrityResource::CheckResourceIntegrity(Document& document) {
  // Already checked? Retain existing result.
  //
  // TODO(vogelheim): If IntegrityDisposition() is kFailed, this should
  // probably also generate a console message identical to the one produced
  // by the CheckSubresourceIntegrity call below. See crbug.com/585267.
  if (IntegrityDisposition() != ResourceIntegrityDisposition::kNotChecked)
    return;

  // Loading error occurred? Then result is uncheckable.
  if (ErrorOccurred())
    return;

  // No integrity attributes to check? Then we're passing.
  if (IntegrityMetadata().IsEmpty()) {
    SetIntegrityDisposition(ResourceIntegrityDisposition::kPassed);
    return;
  }

  CHECK(!!ResourceBuffer());
  bool passed = SubresourceIntegrity::CheckSubresourceIntegrity(
      IntegrityMetadata(), document, ResourceBuffer()->Data(),
      ResourceBuffer()->size(), Url(), *this);
  SetIntegrityDisposition(passed ? ResourceIntegrityDisposition::kPassed
                                 : ResourceIntegrityDisposition::kFailed);
  DCHECK_NE(IntegrityDisposition(), ResourceIntegrityDisposition::kNotChecked);
}

TextIntegrityResource::TextIntegrityResource(
    const ResourceRequest& resource_request,
    Type type,
    const ResourceLoaderOptions& options,
    const TextResourceDecoderOptions& decoder_options)
    : TextResource(resource_request, type, options, decoder_options),
      source_text_(),
      integrity_disposition_(ResourceIntegrityDisposition::kNotChecked) {}

TextIntegrityResource::~TextIntegrityResource() {}

void TextIntegrityResource::ClearSourceText() {
  source_text_ = AtomicString();
}

size_t TextIntegrityResource::SourceTextSizeInBytes() const {
  return source_text_.CharactersSizeInBytes();
}

}  // namespace blink
