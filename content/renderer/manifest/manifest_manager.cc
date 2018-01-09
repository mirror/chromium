// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/manifest/ManifestManager.h"

#include <utility>

#include "bindings/core/v8/SourceLocation.h"
#include "core/frame/FrameConsole.h"
#include "core/frame/LocalFrameClient.h"
#include "core/html/HTMLLinkElement.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/loader/DocumentThreadableLoader.h"
#include "core/loader/DocumentThreadableLoaderClient.h"
#include "core/loader/ThreadableLoadingContext.h"
#include "modules/manifest/ManifestParser.h"
#include "modules/manifest/ManifestUmaUtil.h"
#include "platform/WebTaskRunner.h"
#include "platform/loader/fetch/FetchParameters.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

class ManifestManager::Fetcher : public GarbageCollectedFinalized<Fetcher>,
                                 public ThreadableLoaderClient {
  WTF_MAKE_NONCOPYABLE(Fetcher);
  EAGERLY_FINALIZE();

 public:
  Fetcher(ManifestManager* owner)
      : owner_(owner), buffer_(SharedBuffer::Create()) {}

  ~Fetcher() override {
    if (loader_)
      Cancel();
  }

  void Start(const KURL& url, bool use_credentials) {
    ThreadableLoaderOptions options;
    ResourceLoaderOptions resource_loader_options;
    resource_loader_options.data_buffering_policy = kDoNotBufferData;
    loader_ = DocumentThreadableLoader::Create(
        *ThreadableLoadingContext::Create(owner_->GetDocument()), this, options,
        resource_loader_options);
    ResourceRequest request(url);
    // See https://w3c.github.io/manifest/. Use "include" when use_credentials
    // is true, and "omit" otherwise.
    request.SetFetchCredentialsMode(
        use_credentials ? network::mojom::FetchCredentialsMode::kInclude
                        : network::mojom::FetchCredentialsMode::kOmit);
    request.SetFetchRequestMode(network::mojom::FetchRequestMode::kCORS);
    request.SetRequestContext(WebURLRequest::kRequestContextManifest);
    request.SetFrameType(network::mojom::RequestContextFrameType::kNone);
    request.SetSiteForCookies(owner_->GetDocument().SiteForCookies());
    loader_->Start(request);
  }

  void Cancel() {
    owner_ = nullptr;
    DCHECK(loader_);
    loader_->Cancel();
    loader_ = nullptr;
  }

  void Trace(Visitor* visitor) {
    visitor->Trace(owner_);
    visitor->Trace(loader_);
  }

 private:
  // ThreadableLoaderClient:
  void DidReceiveResponse(unsigned long /*identifier*/,
                          const ResourceResponse& response,
                          std::unique_ptr<WebDataConsumerHandle>) override {
    manifest_url_ = response.Url();
  }

  void DidReceiveData(const char* data, unsigned length) override {
    buffer_->Append(data, length);
  }

  void DidFinishLoading(unsigned long /*identifier*/,
                        double /*finishTime*/) override {
    if (!owner_)
      return;

    auto contiguous_buffer = buffer_->Copy();
    owner_->OnManifestFetchComplete(
        manifest_url_,
        String::FromUTF8(contiguous_buffer.data(), contiguous_buffer.size()));
  }

  void DidFail(const ResourceError&) override {
    if (!owner_)
      return;

    owner_->OnManifestFetchComplete(manifest_url_, String());
  }

  WeakMember<ManifestManager> owner_;
  KURL manifest_url_;
  scoped_refptr<SharedBuffer> buffer_;
  Member<DocumentThreadableLoader> loader_;
};

ManifestManager::ManifestManager(Document& document)
    : Supplement<Document>(document) {}

ManifestManager::~ManifestManager() = default;

void ManifestManager::Trace(Visitor* visitor) {
  Supplement<Document>::Trace(visitor);
  visitor->Trace(fetcher_);
}

// static
const char* ManifestManager::SupplementName() {
  return "ManifestManager";
}

// static
ManifestManager* ManifestManager::From(Document* document) {
  return static_cast<ManifestManager*>(
      Supplement<Document>::From(document, SupplementName()));
}

// static
void ManifestManager::ProvideTo(Document& document) {
  Supplement<Document>::ProvideTo(document, SupplementName(),
                                  new ManifestManager(document));
}

// static
void ManifestManager::BindMojoRequest(
    LocalFrame* frame,
    mojom::blink::ManifestManagerRequest request) {
  if (!frame->GetDocument())
    return;

  From(frame->GetDocument())->BindToRequest(std::move(request));
}

void ManifestManager::RequestManifest(RequestManifestCallback callback) {
  RequestManifestImpl(WTF::Bind(
      [](RequestManifestCallback callback, const KURL& manifest_url,
         const mojom::blink::Manifest* manifest,
         const mojom::blink::ManifestDebugInfo* debug_info) {
        std::move(callback).Run(manifest_url,
                                manifest ? manifest->Clone() : nullptr);
      },
      WTF::Passed(std::move(callback))));
}

void ManifestManager::RequestManifestDebugInfo(
    RequestManifestDebugInfoCallback callback) {
  RequestManifestImpl(WTF::Bind(
      [](RequestManifestDebugInfoCallback callback, const KURL& manifest_url,
         const mojom::blink::Manifest* manifest,
         const mojom::blink::ManifestDebugInfo* debug_info) {
        std::move(callback).Run(manifest_url,
                                debug_info ? debug_info->Clone() : nullptr);
      },
      WTF::Passed(std::move(callback))));
}

void ManifestManager::RequestManifestImpl(
    InternalRequestManifestCallback callback) {
  CheckForManifestChange();
  if (fetcher_) {
    pending_callbacks_.push_back(std::move(callback));
    return;
  }
  std::move(callback).Run(manifest_url_, manifest_.get(),
                          manifest_debug_info_.get());
}

void ManifestManager::FetchManifest() {
  // If a fetch was already in progress, cancel it.
  if (fetcher_) {
    fetcher_->Cancel();
    fetcher_ = nullptr;
  }
  if (document_manifest_url_.IsEmpty()) {
    ManifestUmaUtil::FetchFailed(ManifestUmaUtil::FETCH_EMPTY_URL);
    ResolveCallbacks();
    return;
  }
  fetcher_ = new Fetcher(this);
  fetcher_->Start(document_manifest_url_, UseCredentials());
}

void ManifestManager::OnManifestFetchComplete(const KURL& manifest_url,
                                              const String& data) {
  if (CheckForManifestChange())
    return;

  manifest_url_ = manifest_url.IsNull() ? document_manifest_url_ : manifest_url;
  if (data.IsEmpty()) {
    ManifestUmaUtil::FetchFailed(ManifestUmaUtil::FETCH_UNSPECIFIED_REASON);
    ResolveCallbacks();
    return;
  }

  ManifestUmaUtil::FetchSucceeded();
  ManifestParser parser(data, manifest_url, GetDocument().Url());
  parser.Parse();

  manifest_debug_info_ = mojom::blink::ManifestDebugInfo::New();
  manifest_debug_info_->raw_manifest = data;
  manifest_debug_info_->errors = parser.TakeErrors();

  for (const auto& error : manifest_debug_info_->errors) {
    GetDocument().GetFrame()->Console().AddMessage(ConsoleMessage::Create(
        kOtherMessageSource,
        error->critical ? kErrorMessageLevel : kWarningMessageLevel,
        "Manifest: " + error->message,
        SourceLocation::Create(manifest_url_.GetString(), 0, 0, nullptr)));
  }
  manifest_ = parser.Manifest();
  ResolveCallbacks();
}

void ManifestManager::ResolveCallbacks() {
  if (fetcher_) {
    fetcher_->Cancel();
    fetcher_ = nullptr;
  }

  for (auto& callback : pending_callbacks_) {
    std::move(callback).Run(manifest_url_, manifest_.get(),
                            manifest_debug_info_.get());
  }
  pending_callbacks_.clear();
}

void ManifestManager::BindToRequest(
    mojom::blink::ManifestManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

bool ManifestManager::CheckForManifestChange() {
  KURL current_manifest_url = GetManifestUrl();
  if (current_manifest_url == document_manifest_url_)
    return false;

  document_manifest_url_ = current_manifest_url;
  manifest_url_ = KURL();
  manifest_ = nullptr;
  manifest_debug_info_ = nullptr;
  if (document_manifest_url_.IsNull())
    ResolveCallbacks();
  else
    FetchManifest();
  return true;
}

KURL ManifestManager::GetManifestUrl() {
  HTMLLinkElement* link_element = GetManifestLink();
  if (link_element)
    return link_element->Href();
  return KURL();
}

bool ManifestManager::UseCredentials() {
  HTMLLinkElement* link_element = GetManifestLink();
  DCHECK(link_element);
  return EqualIgnoringASCIICase(
      link_element->FastGetAttribute(HTMLNames::crossoriginAttr),
      "use-credentials");
}

}  // namespace blink
