// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/resource/TextResource.h"

#include "core/html/parser/TextResourceDecoder.h"
#include "platform/SharedBuffer.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/loader/fetch/TextResourceDecoderOptions.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

TextResource* TextResource::FetchScript(FetchParameters& params,
                                        ResourceFetcher* fetcher) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            WebURLRequest::kFrameTypeNone);
  params.SetRequestContext(WebURLRequest::kRequestContextScript);
  return ToTextResource(fetcher->RequestResource(params, Factory(kScript)));
}

static void ApplyXSLRequestProperties(FetchParameters& params) {
  params.SetRequestContext(WebURLRequest::kRequestContextXSLT);
  // TODO(japhet): Accept: headers can be set manually on XHRs from script, in
  // the browser process, and... here. The browser process can't tell the
  // difference between an XSL stylesheet and a CSS stylesheet, so it assumes
  // stylesheets are all CSS unless they already have an Accept: header set.
  // Should we teach the browser process the difference?
  DEFINE_STATIC_LOCAL(const AtomicString, accept_xslt,
                      ("text/xml, application/xml, application/xhtml+xml, "
                       "text/xsl, application/rss+xml, application/atom+xml"));
  params.MutableResourceRequest().SetHTTPAccept(accept_xslt);
}

TextResource* TextResource::FetchXSL(FetchParameters& params,
                                     ResourceFetcher* fetcher) {
  DCHECK(RuntimeEnabledFeatures::XSLTEnabled());
  ApplyXSLRequestProperties(params);
  return ToTextResource(
      fetcher->RequestResource(params, Factory(kXSLStyleSheet)));
}

TextResource* TextResource::FetchXSLSynchronously(FetchParameters& params,
                                                  ResourceFetcher* fetcher) {
  ApplyXSLRequestProperties(params);
  params.MakeSynchronous();
  return ToTextResource(
      fetcher->RequestResource(params, Factory(kXSLStyleSheet)));
}

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

  // Clear raw bytes as now we have the full decoded text.
  // We wait for all NotifyFinished() calls to run (at least once)
  // as SubresourceIntegrity checks require raw bytes.
  ClearData();
}

void TextResource::DestroyDecodedDataForFailedRevalidation() {
  size_t new_size = DecodedSize() - decoded_text_.CharactersSizeInBytes();
  decoded_text_ = String();
  SetDecodedSize(new_size);
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

void TextResource::OnMemoryDump(WebMemoryDumpLevelOfDetail level_of_detail,
                                WebProcessMemoryDump* memory_dump) const {
  Resource::OnMemoryDump(level_of_detail, memory_dump);
  const String name = GetMemoryDumpName() + "/decoded_script";
  auto* dump = memory_dump->CreateMemoryAllocatorDump(name);
  dump->AddScalar("size", "bytes", DecodedText().CharactersSizeInBytes());
  memory_dump->AddSuballocation(
      dump->Guid(), String(WTF::Partitions::kAllocatedObjectPoolName));
}

AccessControlStatus TextResource::CalculateAccessControlStatus() const {
  if (GetCORSStatus() == CORSStatus::kServiceWorkerOpaque)
    return kOpaqueResource;

  if (IsSameOriginOrCORSSuccessful())
    return kSharableCrossOrigin;

  return kNotSharableCrossOrigin;
}

bool TextResource::CanUseCacheValidator() const {
  // Do not revalidate until ClassicPendingScript is removed, i.e. the script
  // content is retrieved in ScriptLoader::ExecuteScriptBlock().
  // crbug.com/692856
  if (Type() == kScript && HasClientsOrObservers())
    return false;

  return Resource::CanUseCacheValidator();
}

}  // namespace blink
