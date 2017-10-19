// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/FetchAPIRequestStructTraits.h"

#include "mojo/public/cpp/bindings/map_traits_wtf_hash_map.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/blob/BlobData.h"
#include "platform/mojo/KURLStructTraits.h"
#include "platform/mojo/ReferrerStructTraits.h"
#include "platform/weborigin/Referrer.h"
#include "public/platform/WebReferrerPolicy.h"

namespace mojo {

using blink::mojom::fetch::CredentialsMode;
using blink::mojom::fetch::RedirectMode;
using blink::mojom::fetch::RequestContextFrameType;
using blink::mojom::fetch::RequestContextType;
using blink::mojom::fetch::RequestMode;

CredentialsMode
EnumTraits<CredentialsMode, blink::WebURLRequest::FetchCredentialsMode>::
    ToMojom(blink::WebURLRequest::FetchCredentialsMode input) {
  switch (input) {
    case blink::WebURLRequest::kFetchCredentialsModeOmit:
      return CredentialsMode::OMIT;
    case blink::WebURLRequest::kFetchCredentialsModeSameOrigin:
      return CredentialsMode::SAME_ORIGIN;
    case blink::WebURLRequest::kFetchCredentialsModeInclude:
      return CredentialsMode::INCLUDE;
    case blink::WebURLRequest::kFetchCredentialsModePassword:
      return CredentialsMode::PASSWORD;
  }

  NOTREACHED();
  return CredentialsMode::OMIT;
}

bool EnumTraits<CredentialsMode, blink::WebURLRequest::FetchCredentialsMode>::
    FromMojom(CredentialsMode input,
              blink::WebURLRequest::FetchCredentialsMode* out) {
  switch (input) {
    case CredentialsMode::OMIT:
      *out = blink::WebURLRequest::kFetchCredentialsModeOmit;
      return true;
    case CredentialsMode::SAME_ORIGIN:
      *out = blink::WebURLRequest::kFetchCredentialsModeSameOrigin;
      return true;
    case CredentialsMode::INCLUDE:
      *out = blink::WebURLRequest::kFetchCredentialsModeInclude;
      return true;
    case CredentialsMode::PASSWORD:
      *out = blink::WebURLRequest::kFetchCredentialsModePassword;
      return true;
  }

  return false;
}

RedirectMode
EnumTraits<RedirectMode, blink::WebURLRequest::FetchRedirectMode>::ToMojom(
    blink::WebURLRequest::FetchRedirectMode input) {
  switch (input) {
    case blink::WebURLRequest::kFetchRedirectModeFollow:
      return RedirectMode::FOLLOW;
    case blink::WebURLRequest::kFetchRedirectModeError:
      return RedirectMode::ERROR_MODE;
    case blink::WebURLRequest::kFetchRedirectModeManual:
      return RedirectMode::MANUAL;
  }

  NOTREACHED();
  return RedirectMode::ERROR_MODE;
}

bool EnumTraits<RedirectMode, blink::WebURLRequest::FetchRedirectMode>::
    FromMojom(RedirectMode input,
              blink::WebURLRequest::FetchRedirectMode* out) {
  switch (input) {
    case RedirectMode::FOLLOW:
      *out = blink::WebURLRequest::kFetchRedirectModeFollow;
      return true;
    case RedirectMode::ERROR_MODE:
      *out = blink::WebURLRequest::kFetchRedirectModeError;
      return true;
    case RedirectMode::MANUAL:
      *out = blink::WebURLRequest::kFetchRedirectModeManual;
      return true;
  }

  return false;
}

RequestMode
EnumTraits<RequestMode, blink::WebURLRequest::FetchRequestMode>::ToMojom(
    blink::WebURLRequest::FetchRequestMode input) {
  switch (input) {
    case blink::WebURLRequest::kFetchRequestModeSameOrigin:
      return RequestMode::SAME_ORIGIN;
    case blink::WebURLRequest::kFetchRequestModeNoCORS:
      return RequestMode::NO_CORS;
    case blink::WebURLRequest::kFetchRequestModeCORS:
      return RequestMode::CORS;
    case blink::WebURLRequest::kFetchRequestModeCORSWithForcedPreflight:
      return RequestMode::CORS_WITH_FORCED_PREFLIGHT;
    case blink::WebURLRequest::kFetchRequestModeNavigate:
      return RequestMode::NAVIGATE;
  }

  NOTREACHED();
  return RequestMode::NO_CORS;
}

bool EnumTraits<RequestMode, blink::WebURLRequest::FetchRequestMode>::FromMojom(
    RequestMode input,
    blink::WebURLRequest::FetchRequestMode* out) {
  switch (input) {
    case RequestMode::SAME_ORIGIN:
      *out = blink::WebURLRequest::kFetchRequestModeSameOrigin;
      return true;
    case RequestMode::NO_CORS:
      *out = blink::WebURLRequest::kFetchRequestModeNoCORS;
      return true;
    case RequestMode::CORS:
      *out = blink::WebURLRequest::kFetchRequestModeCORS;
      return true;
    case RequestMode::CORS_WITH_FORCED_PREFLIGHT:
      *out = blink::WebURLRequest::kFetchRequestModeCORSWithForcedPreflight;
      return true;
    case RequestMode::NAVIGATE:
      *out = blink::WebURLRequest::kFetchRequestModeNavigate;
      return true;
  }

  return false;
}

RequestContextFrameType
EnumTraits<RequestContextFrameType, blink::WebURLRequest::FrameType>::ToMojom(
    blink::WebURLRequest::FrameType input) {
  switch (input) {
    case blink::WebURLRequest::kFrameTypeAuxiliary:
      return RequestContextFrameType::AUXILIARY;
    case blink::WebURLRequest::kFrameTypeNested:
      return RequestContextFrameType::NESTED;
    case blink::WebURLRequest::kFrameTypeNone:
      return RequestContextFrameType::NONE;
    case blink::WebURLRequest::kFrameTypeTopLevel:
      return RequestContextFrameType::TOP_LEVEL;
  }

  NOTREACHED();
  return RequestContextFrameType::NONE;
}

bool EnumTraits<RequestContextFrameType, blink::WebURLRequest::FrameType>::
    FromMojom(RequestContextFrameType input,
              blink::WebURLRequest::FrameType* out) {
  switch (input) {
    case RequestContextFrameType::AUXILIARY:
      *out = blink::WebURLRequest::kFrameTypeAuxiliary;
      return true;
    case RequestContextFrameType::NESTED:
      *out = blink::WebURLRequest::kFrameTypeNested;
      return true;
    case RequestContextFrameType::NONE:
      *out = blink::WebURLRequest::kFrameTypeNone;
      return true;
    case RequestContextFrameType::TOP_LEVEL:
      *out = blink::WebURLRequest::kFrameTypeTopLevel;
      return true;
  }

  return false;
}

RequestContextType
EnumTraits<RequestContextType, blink::WebURLRequest::RequestContext>::ToMojom(
    blink::WebURLRequest::RequestContext input) {
  switch (input) {
    case blink::WebURLRequest::kRequestContextUnspecified:
      return RequestContextType::UNSPECIFIED;
    case blink::WebURLRequest::kRequestContextAudio:
      return RequestContextType::AUDIO;
    case blink::WebURLRequest::kRequestContextBeacon:
      return RequestContextType::BEACON;
    case blink::WebURLRequest::kRequestContextCSPReport:
      return RequestContextType::CSP_REPORT;
    case blink::WebURLRequest::kRequestContextDownload:
      return RequestContextType::DOWNLOAD;
    case blink::WebURLRequest::kRequestContextEmbed:
      return RequestContextType::EMBED;
    case blink::WebURLRequest::kRequestContextEventSource:
      return RequestContextType::EVENT_SOURCE;
    case blink::WebURLRequest::kRequestContextFavicon:
      return RequestContextType::FAVICON;
    case blink::WebURLRequest::kRequestContextFetch:
      return RequestContextType::FETCH;
    case blink::WebURLRequest::kRequestContextFont:
      return RequestContextType::FONT;
    case blink::WebURLRequest::kRequestContextForm:
      return RequestContextType::FORM;
    case blink::WebURLRequest::kRequestContextFrame:
      return RequestContextType::FRAME;
    case blink::WebURLRequest::kRequestContextHyperlink:
      return RequestContextType::HYPERLINK;
    case blink::WebURLRequest::kRequestContextIframe:
      return RequestContextType::IFRAME;
    case blink::WebURLRequest::kRequestContextImage:
      return RequestContextType::IMAGE;
    case blink::WebURLRequest::kRequestContextImageSet:
      return RequestContextType::IMAGE_SET;
    case blink::WebURLRequest::kRequestContextImport:
      return RequestContextType::IMPORT;
    case blink::WebURLRequest::kRequestContextInternal:
      return RequestContextType::INTERNAL;
    case blink::WebURLRequest::kRequestContextLocation:
      return RequestContextType::LOCATION;
    case blink::WebURLRequest::kRequestContextManifest:
      return RequestContextType::MANIFEST;
    case blink::WebURLRequest::kRequestContextObject:
      return RequestContextType::OBJECT;
    case blink::WebURLRequest::kRequestContextPing:
      return RequestContextType::PING;
    case blink::WebURLRequest::kRequestContextPlugin:
      return RequestContextType::PLUGIN;
    case blink::WebURLRequest::kRequestContextPrefetch:
      return RequestContextType::PREFETCH;
    case blink::WebURLRequest::kRequestContextScript:
      return RequestContextType::SCRIPT;
    case blink::WebURLRequest::kRequestContextServiceWorker:
      return RequestContextType::SERVICE_WORKER;
    case blink::WebURLRequest::kRequestContextSharedWorker:
      return RequestContextType::SHARED_WORKER;
    case blink::WebURLRequest::kRequestContextSubresource:
      return RequestContextType::SUBRESOURCE;
    case blink::WebURLRequest::kRequestContextStyle:
      return RequestContextType::STYLE;
    case blink::WebURLRequest::kRequestContextTrack:
      return RequestContextType::TRACK;
    case blink::WebURLRequest::kRequestContextVideo:
      return RequestContextType::VIDEO;
    case blink::WebURLRequest::kRequestContextWorker:
      return RequestContextType::WORKER;
    case blink::WebURLRequest::kRequestContextXMLHttpRequest:
      return RequestContextType::XML_HTTP_REQUEST;
    case blink::WebURLRequest::kRequestContextXSLT:
      return RequestContextType::XSLT;
  }

  NOTREACHED();
  return RequestContextType::UNSPECIFIED;
}

bool EnumTraits<RequestContextType, blink::WebURLRequest::RequestContext>::
    FromMojom(RequestContextType input,
              blink::WebURLRequest::RequestContext* out) {
  switch (input) {
    case RequestContextType::UNSPECIFIED:
      *out = blink::WebURLRequest::kRequestContextUnspecified;
      return true;
    case RequestContextType::AUDIO:
      *out = blink::WebURLRequest::kRequestContextAudio;
      return true;
    case RequestContextType::BEACON:
      *out = blink::WebURLRequest::kRequestContextBeacon;
      return true;
    case RequestContextType::CSP_REPORT:
      *out = blink::WebURLRequest::kRequestContextCSPReport;
      return true;
    case RequestContextType::DOWNLOAD:
      *out = blink::WebURLRequest::kRequestContextDownload;
      return true;
    case RequestContextType::EMBED:
      *out = blink::WebURLRequest::kRequestContextEmbed;
      return true;
    case RequestContextType::EVENT_SOURCE:
      *out = blink::WebURLRequest::kRequestContextEventSource;
      return true;
    case RequestContextType::FAVICON:
      *out = blink::WebURLRequest::kRequestContextFavicon;
      return true;
    case RequestContextType::FETCH:
      *out = blink::WebURLRequest::kRequestContextFetch;
      return true;
    case RequestContextType::FONT:
      *out = blink::WebURLRequest::kRequestContextFont;
      return true;
    case RequestContextType::FORM:
      *out = blink::WebURLRequest::kRequestContextForm;
      return true;
    case RequestContextType::FRAME:
      *out = blink::WebURLRequest::kRequestContextFrame;
      return true;
    case RequestContextType::HYPERLINK:
      *out = blink::WebURLRequest::kRequestContextHyperlink;
      return true;
    case RequestContextType::IFRAME:
      *out = blink::WebURLRequest::kRequestContextIframe;
      return true;
    case RequestContextType::IMAGE:
      *out = blink::WebURLRequest::kRequestContextImage;
      return true;
    case RequestContextType::IMAGE_SET:
      *out = blink::WebURLRequest::kRequestContextImageSet;
      return true;
    case RequestContextType::IMPORT:
      *out = blink::WebURLRequest::kRequestContextImport;
      return true;
    case RequestContextType::INTERNAL:
      *out = blink::WebURLRequest::kRequestContextInternal;
      return true;
    case RequestContextType::LOCATION:
      *out = blink::WebURLRequest::kRequestContextLocation;
      return true;
    case RequestContextType::MANIFEST:
      *out = blink::WebURLRequest::kRequestContextManifest;
      return true;
    case RequestContextType::OBJECT:
      *out = blink::WebURLRequest::kRequestContextObject;
      return true;
    case RequestContextType::PING:
      *out = blink::WebURLRequest::kRequestContextPing;
      return true;
    case RequestContextType::PLUGIN:
      *out = blink::WebURLRequest::kRequestContextPlugin;
      return true;
    case RequestContextType::PREFETCH:
      *out = blink::WebURLRequest::kRequestContextPrefetch;
      return true;
    case RequestContextType::SCRIPT:
      *out = blink::WebURLRequest::kRequestContextScript;
      return true;
    case RequestContextType::SERVICE_WORKER:
      *out = blink::WebURLRequest::kRequestContextServiceWorker;
      return true;
    case RequestContextType::SHARED_WORKER:
      *out = blink::WebURLRequest::kRequestContextSharedWorker;
      return true;
    case RequestContextType::SUBRESOURCE:
      *out = blink::WebURLRequest::kRequestContextSubresource;
      return true;
    case RequestContextType::STYLE:
      *out = blink::WebURLRequest::kRequestContextStyle;
      return true;
    case RequestContextType::TRACK:
      *out = blink::WebURLRequest::kRequestContextTrack;
      return true;
    case RequestContextType::VIDEO:
      *out = blink::WebURLRequest::kRequestContextVideo;
      return true;
    case RequestContextType::WORKER:
      *out = blink::WebURLRequest::kRequestContextWorker;
      return true;
    case RequestContextType::XML_HTTP_REQUEST:
      *out = blink::WebURLRequest::kRequestContextXMLHttpRequest;
      return true;
    case RequestContextType::XSLT:
      *out = blink::WebURLRequest::kRequestContextXSLT;
      return true;
  }

  return false;
}

// static
blink::KURL StructTraits<blink::mojom::fetch::RequestDataView,
                         blink::WebServiceWorkerRequest>::
    url(const blink::WebServiceWorkerRequest& request) {
  return request.Url();
}

// static
WTF::String StructTraits<blink::mojom::fetch::RequestDataView,
                         blink::WebServiceWorkerRequest>::
    method(const blink::WebServiceWorkerRequest& request) {
  return request.Method();
}

// static
WTF::HashMap<WTF::String, WTF::String>
StructTraits<blink::mojom::fetch::RequestDataView,
             blink::WebServiceWorkerRequest>::
    headers(const blink::WebServiceWorkerRequest& request) {
  WTF::HashMap<WTF::String, WTF::String> header_map;
  for (const auto& pair : request.Headers())
    header_map.insert(pair.key, pair.value);
  return header_map;
}

// static
const blink::Referrer& StructTraits<blink::mojom::fetch::RequestDataView,
                                    blink::WebServiceWorkerRequest>::
    referrer(const blink::WebServiceWorkerRequest& request) {
  return request.GetReferrer();
}

// static
WTF::String StructTraits<blink::mojom::fetch::RequestDataView,
                         blink::WebServiceWorkerRequest>::
    blob_uuid(const blink::WebServiceWorkerRequest& request) {
  if (request.GetBlobDataHandle())
    return request.GetBlobDataHandle()->Uuid();

  return WTF::String();
}

// static
uint64_t StructTraits<blink::mojom::fetch::RequestDataView,
                      blink::WebServiceWorkerRequest>::
    blob_size(const blink::WebServiceWorkerRequest& request) {
  if (request.GetBlobDataHandle())
    return request.GetBlobDataHandle()->size();

  return 0;
}

// static
storage::mojom::blink::BlobPtr StructTraits<
    blink::mojom::fetch::RequestDataView,
    blink::WebServiceWorkerRequest>::blob(const blink::WebServiceWorkerRequest&
                                              request) {
  if (request.GetBlobDataHandle()) {
    storage::mojom::blink::BlobPtr result =
        request.GetBlobDataHandle()->CloneBlobPtr();
    return result;
  }

  return nullptr;
}

// static
WTF::String StructTraits<blink::mojom::fetch::RequestDataView,
                         blink::WebServiceWorkerRequest>::
    integrity(const blink::WebServiceWorkerRequest& request) {
  return request.Integrity();
}

// static
WTF::String StructTraits<blink::mojom::fetch::RequestDataView,
                         blink::WebServiceWorkerRequest>::
    client_id(const blink::WebServiceWorkerRequest& request) {
  return request.ClientId();
}

// static
bool StructTraits<blink::mojom::fetch::RequestDataView,
                  blink::WebServiceWorkerRequest>::
    Read(blink::mojom::fetch::RequestDataView data,
         blink::WebServiceWorkerRequest* out) {
  blink::WebURLRequest::FetchRequestMode mode;
  blink::WebURLRequest::RequestContext requestContext;
  blink::WebURLRequest::FrameType frameType;
  blink::KURL url;
  WTF::String method;
  WTF::HashMap<WTF::String, WTF::String> headers;
  WTF::String blobUuid;
  storage::mojom::blink::BlobPtr blob;
  blink::Referrer referrer;
  blink::WebURLRequest::FetchCredentialsMode credentialsMode;
  blink::WebURLRequest::FetchRedirectMode redirectMode;
  WTF::String integrity;
  WTF::String clientId;

  if (!data.ReadMode(&mode) || !data.ReadRequestContextType(&requestContext) ||
      !data.ReadFrameType(&frameType) || !data.ReadUrl(&url) ||
      !data.ReadMethod(&method) || !data.ReadHeaders(&headers) ||
      !data.ReadBlobUuid(&blobUuid) || !data.ReadReferrer(&referrer) ||
      !data.ReadCredentialsMode(&credentialsMode) ||
      !data.ReadRedirectMode(&redirectMode) || !data.ReadClientId(&clientId) ||
      !data.ReadIntegrity(&integrity)) {
    return false;
  }

  out->SetMode(mode);
  out->SetIsMainResourceLoad(data.is_main_resource_load());
  out->SetRequestContext(requestContext);
  out->SetFrameType(frameType);
  out->SetURL(url);
  out->SetMethod(method);
  for (const auto& pair : headers)
    out->SetHeader(pair.key, pair.value);
  out->SetBlob(blobUuid, static_cast<long long>(data.blob_size()),
               data.TakeBlob<storage::mojom::blink::BlobPtr>().PassInterface());
  out->SetReferrer(referrer.referrer, static_cast<blink::WebReferrerPolicy>(
                                          referrer.referrer_policy));
  out->SetCredentialsMode(credentialsMode);
  out->SetRedirectMode(redirectMode);
  out->SetIntegrity(integrity);
  out->SetClientId(clientId);
  out->SetIsReload(data.is_reload());
  return true;
}

// static
bool StructTraits<blink::mojom::fetch::ResponseDataView,
                  blink::WebServiceWorkerResponse>::
    Read(blink::mojom::fetch::ResponseDataView data,
         blink::WebServiceWorkerResponse* out) {
  NOTREACHED() << "Implement WebServiceWorkerResponse struct traits";
#if 0
  blink::WebURLResponse::FetchResponseMode mode;
  blink::WebURLResponse::ResponseContext responseContext;
  blink::WebURLResponse::FrameType frameType;
  blink::KURL url;
  WTF::String method;
  WTF::HashMap<WTF::String, WTF::String> headers;
  WTF::String blobUuid;
  blink::Referrer referrer;
  blink::WebURLResponse::FetchCredentialsMode credentialsMode;
  blink::WebURLResponse::FetchRedirectMode redirectMode;
  WTF::String clientId;

  if (!data.ReadMode(&mode) || !data.ReadResponseContextType(&responseContext) ||
      !data.ReadFrameType(&frameType) || !data.ReadUrl(&url) ||
      !data.ReadMethod(&method) || !data.ReadHeaders(&headers) ||
      !data.ReadBlobUuid(&blobUuid) || !data.ReadReferrer(&referrer) ||
      !data.ReadCredentialsMode(&credentialsMode) ||
      !data.ReadRedirectMode(&redirectMode) || !data.ReadClientId(&clientId)) {
    return false;
  }

  out->SetMode(mode);
  out->SetIsMainResourceLoad(data.is_main_resource_load());
  out->SetResponseContext(responseContext);
  out->SetFrameType(frameType);
  out->SetURL(url);
  out->SetMethod(method);
  for (const auto& pair : headers)
    out->SetHeader(pair.key, pair.value);
  out->SetBlob(blobUuid, static_cast<long long>(data.blob_size()));
  out->SetReferrer(referrer.referrer, static_cast<blink::WebReferrerPolicy>(
                                          referrer.referrer_policy));
  out->SetCredentialsMode(credentialsMode);
  out->SetRedirectMode(redirectMode);
  out->SetClientId(clientId);
  out->SetIsReload(data.is_reload());
#endif
  return true;
}

// static
WTF::Vector<blink::KURL> StructTraits<blink::mojom::fetch::ResponseDataView,
                                      blink::WebServiceWorkerResponse>::
    url_list(const blink::WebServiceWorkerResponse& response) {
  const blink::WebVector<blink::WebURL>& response_urls = response.UrlList();
  WTF::Vector<blink::KURL> urls(response_urls.size());
  for (size_t i = 0; i < response_urls.size(); i++)
    urls[i] = response_urls[i];
  return urls;
}

WTF::String StructTraits<blink::mojom::fetch::ResponseDataView,
                         blink::WebServiceWorkerResponse>::
    status_text(const blink::WebServiceWorkerResponse& response) {
  return response.StatusText();
}

WTF::HashMap<WTF::String, WTF::String>
StructTraits<blink::mojom::fetch::ResponseDataView,
             blink::WebServiceWorkerResponse>::
    headers(const blink::WebServiceWorkerResponse& response) {
  WTF::HashMap<WTF::String, WTF::String> headers;

  for (const blink::WebString& key : response.GetHeaderKeys())
    headers.Set(key, response.GetHeader(key));

  return headers;
}

WTF::String StructTraits<blink::mojom::fetch::ResponseDataView,
                         blink::WebServiceWorkerResponse>::
    blob_uuid(const blink::WebServiceWorkerResponse& response) {
  return response.BlobUUID();
}

WTF::String StructTraits<blink::mojom::fetch::ResponseDataView,
                         blink::WebServiceWorkerResponse>::
    cache_storage_cache_name(const blink::WebServiceWorkerResponse& response) {
  return response.CacheStorageCacheName();
}

WTF::Vector<WTF::String> StructTraits<blink::mojom::fetch::ResponseDataView,
                                      blink::WebServiceWorkerResponse>::
    cors_exposed_header_names(const blink::WebServiceWorkerResponse& response) {
  const blink::WebVector<blink::WebString>& response_names =
      response.CorsExposedHeaderNames();
  WTF::Vector<WTF::String> names(response_names.size());
  for (size_t i = 0; i < response_names.size(); i++)
    names[i] = response_names[i];
  return names;
}

}  // namespace mojo
