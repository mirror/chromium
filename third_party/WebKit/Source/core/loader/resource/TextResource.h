// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextResource_h
#define TextResource_h

#include <memory>
#include "core/CoreExport.h"
#include "core/html/parser/TextResourceDecoder.h"
#include "platform/loader/fetch/AccessControlStatus.h"
#include "platform/loader/fetch/Resource.h"
#include "platform/loader/fetch/ResourceClient.h"
#include "platform/loader/fetch/TextResourceDecoderOptions.h"

namespace blink {

class CORE_EXPORT TextResource : public Resource {
 public:
  using ClientType = ResourceClient;

  static TextResource* FetchScript(FetchParameters&, ResourceFetcher*);
  static TextResource* FetchXSL(FetchParameters&, ResourceFetcher*);
  static TextResource* FetchXSLSynchronously(FetchParameters&,
                                             ResourceFetcher*);

  // Public for testing
  static TextResource* CreateScriptForTest(const KURL& url,
                                           const WTF::TextEncoding& encoding) {
    ResourceRequest request(url);
    request.SetFetchCredentialsMode(
        network::mojom::FetchCredentialsMode::kOmit);
    ResourceLoaderOptions options;
    TextResourceDecoderOptions decoder_options(
        TextResourceDecoderOptions::kPlainTextContent, encoding);
    return new TextResource(request, kScript, options, decoder_options);
  }

  // Returns the decoded data in text form. The data has to be available at
  // call time.
  String DecodedText() const;

  WTF::TextEncoding Encoding() const override;

  void SetEncodingForTest(const String& encoding) { SetEncoding(encoding); }
  void NotifyFinished() override;

  void DestroyDecodedDataForFailedRevalidation() override;

  void OnMemoryDump(WebMemoryDumpLevelOfDetail,
                    WebProcessMemoryDump*) const override;

  AccessControlStatus CalculateAccessControlStatus() const;

 protected:
  TextResource(const ResourceRequest&,
               Type,
               const ResourceLoaderOptions&,
               const TextResourceDecoderOptions&);
  ~TextResource() override;

  void SetEncoding(const String&) override;

 private:
  class Factory : public ResourceFactory {
   public:
    Factory(Type type)
        : ResourceFactory(type, TextResourceDecoderOptions::kPlainTextContent) {
    }

    Resource* Create(
        const ResourceRequest& request,
        const ResourceLoaderOptions& options,
        const TextResourceDecoderOptions& decoder_options) const override {
      return new TextResource(request, type_, options, decoder_options);
    }
  };

  bool CanUseCacheValidator() const override;

  std::unique_ptr<TextResourceDecoder> decoder_;
  String decoded_text_;
};

inline bool IsTextResource(const Resource& resource) {
  Resource::Type type = resource.GetType();
  return type == Resource::kCSSStyleSheet || type == Resource::kSVGDocument ||
         type == Resource::kScript || type == Resource::kXSLStyleSheet;
}

inline TextResource* ToTextResource(Resource* resource) {
  SECURITY_DCHECK(!resource || IsTextResource(*resource));
  return static_cast<TextResource*>(resource);
}

}  // namespace blink

#endif  // TextResource_h
