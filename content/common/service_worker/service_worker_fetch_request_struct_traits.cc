// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_fetch_request_struct_traits.h"

#include "base/logging.h"
#include "content/public/common/referrer_struct_traits.h"
#include "ipc/ipc_message_utils.h"
#include "url/mojo/url_gurl_struct_traits.h"

namespace mojo {

using blink::mojom::fetch::CredentialsMode;
using blink::mojom::fetch::RedirectMode;
using blink::mojom::fetch::RequestContextFrameType;
using blink::mojom::fetch::RequestContextType;
using blink::mojom::fetch::RequestMode;
using blink::mojom::fetch::ServiceWorkerFetchType;

CredentialsMode
EnumTraits<CredentialsMode, content::FetchCredentialsMode>::ToMojom(
    content::FetchCredentialsMode input) {
  switch (input) {
    case content::FETCH_CREDENTIALS_MODE_OMIT:
      return CredentialsMode::OMIT;
    case content::FETCH_CREDENTIALS_MODE_SAME_ORIGIN:
      return CredentialsMode::SAME_ORIGIN;
    case content::FETCH_CREDENTIALS_MODE_INCLUDE:
      return CredentialsMode::INCLUDE;
    case content::FETCH_CREDENTIALS_MODE_PASSWORD:
      return CredentialsMode::PASSWORD;
  }

  NOTREACHED();
  return CredentialsMode::OMIT;
}

bool EnumTraits<CredentialsMode, content::FetchCredentialsMode>::FromMojom(
    CredentialsMode input,
    content::FetchCredentialsMode* out) {
  switch (input) {
    case CredentialsMode::OMIT:
      *out = content::FETCH_CREDENTIALS_MODE_OMIT;
      return true;
    case CredentialsMode::SAME_ORIGIN:
      *out = content::FETCH_CREDENTIALS_MODE_SAME_ORIGIN;
      return true;
    case CredentialsMode::INCLUDE:
      *out = content::FETCH_CREDENTIALS_MODE_INCLUDE;
      return true;
    case CredentialsMode::PASSWORD:
      *out = content::FETCH_CREDENTIALS_MODE_PASSWORD;
      return true;
  }

  return false;
}

RedirectMode EnumTraits<RedirectMode, content::FetchRedirectMode>::ToMojom(
    content::FetchRedirectMode input) {
  switch (input) {
    case content::FetchRedirectMode::FOLLOW_MODE:
      return RedirectMode::FOLLOW;
    case content::FetchRedirectMode::ERROR_MODE:
      return RedirectMode::ERROR_MODE;
    case content::FetchRedirectMode::MANUAL_MODE:
      return RedirectMode::MANUAL;
  }

  NOTREACHED();
  return RedirectMode::ERROR_MODE;
}

bool EnumTraits<RedirectMode, content::FetchRedirectMode>::FromMojom(
    RedirectMode input,
    content::FetchRedirectMode* out) {
  switch (input) {
    case RedirectMode::FOLLOW:
      *out = content::FetchRedirectMode::FOLLOW_MODE;
      return true;
    case RedirectMode::ERROR_MODE:
      *out = content::FetchRedirectMode::ERROR_MODE;
      return true;
    case RedirectMode::MANUAL:
      *out = content::FetchRedirectMode::MANUAL_MODE;
      return true;
  }

  return false;
}

RequestMode EnumTraits<RequestMode, content::FetchRequestMode>::ToMojom(
    content::FetchRequestMode input) {
  switch (input) {
    case content::FETCH_REQUEST_MODE_SAME_ORIGIN:
      return RequestMode::SAME_ORIGIN;
    case content::FETCH_REQUEST_MODE_NO_CORS:
      return RequestMode::NO_CORS;
    case content::FETCH_REQUEST_MODE_CORS:
      return RequestMode::CORS;
    case content::FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT:
      return RequestMode::CORS_WITH_FORCED_PREFLIGHT;
    case content::FETCH_REQUEST_MODE_NAVIGATE:
      return RequestMode::NAVIGATE;
  }

  NOTREACHED();
  return RequestMode::NO_CORS;
}

bool EnumTraits<RequestMode, content::FetchRequestMode>::FromMojom(
    RequestMode input,
    content::FetchRequestMode* out) {
  switch (input) {
    case RequestMode::SAME_ORIGIN:
      *out = content::FETCH_REQUEST_MODE_SAME_ORIGIN;
      return true;
    case RequestMode::NO_CORS:
      *out = content::FETCH_REQUEST_MODE_NO_CORS;
      return true;
    case RequestMode::CORS:
      *out = content::FETCH_REQUEST_MODE_CORS;
      return true;
    case RequestMode::CORS_WITH_FORCED_PREFLIGHT:
      *out = content::FETCH_REQUEST_MODE_CORS_WITH_FORCED_PREFLIGHT;
      return true;
    case RequestMode::NAVIGATE:
      *out = content::FETCH_REQUEST_MODE_NAVIGATE;
      return true;
  }

  return false;
}

RequestContextFrameType
EnumTraits<RequestContextFrameType, content::RequestContextFrameType>::ToMojom(
    content::RequestContextFrameType input) {
  switch (input) {
    case content::REQUEST_CONTEXT_FRAME_TYPE_AUXILIARY:
      return RequestContextFrameType::AUXILIARY;
    case content::REQUEST_CONTEXT_FRAME_TYPE_NESTED:
      return RequestContextFrameType::NESTED;
    case content::REQUEST_CONTEXT_FRAME_TYPE_NONE:
      return RequestContextFrameType::NONE;
    case content::REQUEST_CONTEXT_FRAME_TYPE_TOP_LEVEL:
      return RequestContextFrameType::TOP_LEVEL;
  }

  NOTREACHED();
  return RequestContextFrameType::NONE;
}

bool EnumTraits<RequestContextFrameType, content::RequestContextFrameType>::
    FromMojom(RequestContextFrameType input,
              content::RequestContextFrameType* out) {
  switch (input) {
    case RequestContextFrameType::AUXILIARY:
      *out = content::REQUEST_CONTEXT_FRAME_TYPE_AUXILIARY;
      return true;
    case RequestContextFrameType::NESTED:
      *out = content::REQUEST_CONTEXT_FRAME_TYPE_NESTED;
      return true;
    case RequestContextFrameType::NONE:
      *out = content::REQUEST_CONTEXT_FRAME_TYPE_NONE;
      return true;
    case RequestContextFrameType::TOP_LEVEL:
      *out = content::REQUEST_CONTEXT_FRAME_TYPE_TOP_LEVEL;
      return true;
  }

  return false;
}

RequestContextType
EnumTraits<RequestContextType, content::RequestContextType>::ToMojom(
    content::RequestContextType input) {
  switch (input) {
    case content::REQUEST_CONTEXT_TYPE_UNSPECIFIED:
      return RequestContextType::UNSPECIFIED;
    case content::REQUEST_CONTEXT_TYPE_AUDIO:
      return RequestContextType::AUDIO;
    case content::REQUEST_CONTEXT_TYPE_BEACON:
      return RequestContextType::BEACON;
    case content::REQUEST_CONTEXT_TYPE_CSP_REPORT:
      return RequestContextType::CSP_REPORT;
    case content::REQUEST_CONTEXT_TYPE_DOWNLOAD:
      return RequestContextType::DOWNLOAD;
    case content::REQUEST_CONTEXT_TYPE_EMBED:
      return RequestContextType::EMBED;
    case content::REQUEST_CONTEXT_TYPE_EVENT_SOURCE:
      return RequestContextType::EVENT_SOURCE;
    case content::REQUEST_CONTEXT_TYPE_FAVICON:
      return RequestContextType::FAVICON;
    case content::REQUEST_CONTEXT_TYPE_FETCH:
      return RequestContextType::FETCH;
    case content::REQUEST_CONTEXT_TYPE_FONT:
      return RequestContextType::FONT;
    case content::REQUEST_CONTEXT_TYPE_FORM:
      return RequestContextType::FORM;
    case content::REQUEST_CONTEXT_TYPE_FRAME:
      return RequestContextType::FRAME;
    case content::REQUEST_CONTEXT_TYPE_HYPERLINK:
      return RequestContextType::HYPERLINK;
    case content::REQUEST_CONTEXT_TYPE_IFRAME:
      return RequestContextType::IFRAME;
    case content::REQUEST_CONTEXT_TYPE_IMAGE:
      return RequestContextType::IMAGE;
    case content::REQUEST_CONTEXT_TYPE_IMAGE_SET:
      return RequestContextType::IMAGE_SET;
    case content::REQUEST_CONTEXT_TYPE_IMPORT:
      return RequestContextType::IMPORT;
    case content::REQUEST_CONTEXT_TYPE_INTERNAL:
      return RequestContextType::INTERNAL;
    case content::REQUEST_CONTEXT_TYPE_LOCATION:
      return RequestContextType::LOCATION;
    case content::REQUEST_CONTEXT_TYPE_MANIFEST:
      return RequestContextType::MANIFEST;
    case content::REQUEST_CONTEXT_TYPE_OBJECT:
      return RequestContextType::OBJECT;
    case content::REQUEST_CONTEXT_TYPE_PING:
      return RequestContextType::PING;
    case content::REQUEST_CONTEXT_TYPE_PLUGIN:
      return RequestContextType::PLUGIN;
    case content::REQUEST_CONTEXT_TYPE_PREFETCH:
      return RequestContextType::PREFETCH;
    case content::REQUEST_CONTEXT_TYPE_SCRIPT:
      return RequestContextType::SCRIPT;
    case content::REQUEST_CONTEXT_TYPE_SERVICE_WORKER:
      return RequestContextType::SERVICE_WORKER;
    case content::REQUEST_CONTEXT_TYPE_SHARED_WORKER:
      return RequestContextType::SHARED_WORKER;
    case content::REQUEST_CONTEXT_TYPE_SUBRESOURCE:
      return RequestContextType::SUBRESOURCE;
    case content::REQUEST_CONTEXT_TYPE_STYLE:
      return RequestContextType::STYLE;
    case content::REQUEST_CONTEXT_TYPE_TRACK:
      return RequestContextType::TRACK;
    case content::REQUEST_CONTEXT_TYPE_VIDEO:
      return RequestContextType::VIDEO;
    case content::REQUEST_CONTEXT_TYPE_WORKER:
      return RequestContextType::WORKER;
    case content::REQUEST_CONTEXT_TYPE_XML_HTTP_REQUEST:
      return RequestContextType::XML_HTTP_REQUEST;
    case content::REQUEST_CONTEXT_TYPE_XSLT:
      return RequestContextType::XSLT;
  }

  NOTREACHED();
  return RequestContextType::UNSPECIFIED;
}

bool EnumTraits<RequestContextType, content::RequestContextType>::FromMojom(
    RequestContextType input,
    content::RequestContextType* out) {
  switch (input) {
    case RequestContextType::UNSPECIFIED:
      *out = content::REQUEST_CONTEXT_TYPE_UNSPECIFIED;
      return true;
    case RequestContextType::AUDIO:
      *out = content::REQUEST_CONTEXT_TYPE_AUDIO;
      return true;
    case RequestContextType::BEACON:
      *out = content::REQUEST_CONTEXT_TYPE_BEACON;
      return true;
    case RequestContextType::CSP_REPORT:
      *out = content::REQUEST_CONTEXT_TYPE_CSP_REPORT;
      return true;
    case RequestContextType::DOWNLOAD:
      *out = content::REQUEST_CONTEXT_TYPE_DOWNLOAD;
      return true;
    case RequestContextType::EMBED:
      *out = content::REQUEST_CONTEXT_TYPE_EMBED;
      return true;
    case RequestContextType::EVENT_SOURCE:
      *out = content::REQUEST_CONTEXT_TYPE_EVENT_SOURCE;
      return true;
    case RequestContextType::FAVICON:
      *out = content::REQUEST_CONTEXT_TYPE_FAVICON;
      return true;
    case RequestContextType::FETCH:
      *out = content::REQUEST_CONTEXT_TYPE_FETCH;
      return true;
    case RequestContextType::FONT:
      *out = content::REQUEST_CONTEXT_TYPE_FONT;
      return true;
    case RequestContextType::FORM:
      *out = content::REQUEST_CONTEXT_TYPE_FORM;
      return true;
    case RequestContextType::FRAME:
      *out = content::REQUEST_CONTEXT_TYPE_FRAME;
      return true;
    case RequestContextType::HYPERLINK:
      *out = content::REQUEST_CONTEXT_TYPE_HYPERLINK;
      return true;
    case RequestContextType::IFRAME:
      *out = content::REQUEST_CONTEXT_TYPE_IFRAME;
      return true;
    case RequestContextType::IMAGE:
      *out = content::REQUEST_CONTEXT_TYPE_IMAGE;
      return true;
    case RequestContextType::IMAGE_SET:
      *out = content::REQUEST_CONTEXT_TYPE_IMAGE_SET;
      return true;
    case RequestContextType::IMPORT:
      *out = content::REQUEST_CONTEXT_TYPE_IMPORT;
      return true;
    case RequestContextType::INTERNAL:
      *out = content::REQUEST_CONTEXT_TYPE_INTERNAL;
      return true;
    case RequestContextType::LOCATION:
      *out = content::REQUEST_CONTEXT_TYPE_LOCATION;
      return true;
    case RequestContextType::MANIFEST:
      *out = content::REQUEST_CONTEXT_TYPE_MANIFEST;
      return true;
    case RequestContextType::OBJECT:
      *out = content::REQUEST_CONTEXT_TYPE_OBJECT;
      return true;
    case RequestContextType::PING:
      *out = content::REQUEST_CONTEXT_TYPE_PING;
      return true;
    case RequestContextType::PLUGIN:
      *out = content::REQUEST_CONTEXT_TYPE_PLUGIN;
      return true;
    case RequestContextType::PREFETCH:
      *out = content::REQUEST_CONTEXT_TYPE_PREFETCH;
      return true;
    case RequestContextType::SCRIPT:
      *out = content::REQUEST_CONTEXT_TYPE_SCRIPT;
      return true;
    case RequestContextType::SERVICE_WORKER:
      *out = content::REQUEST_CONTEXT_TYPE_SERVICE_WORKER;
      return true;
    case RequestContextType::SHARED_WORKER:
      *out = content::REQUEST_CONTEXT_TYPE_SHARED_WORKER;
      return true;
    case RequestContextType::SUBRESOURCE:
      *out = content::REQUEST_CONTEXT_TYPE_SUBRESOURCE;
      return true;
    case RequestContextType::STYLE:
      *out = content::REQUEST_CONTEXT_TYPE_STYLE;
      return true;
    case RequestContextType::TRACK:
      *out = content::REQUEST_CONTEXT_TYPE_TRACK;
      return true;
    case RequestContextType::VIDEO:
      *out = content::REQUEST_CONTEXT_TYPE_VIDEO;
      return true;
    case RequestContextType::WORKER:
      *out = content::REQUEST_CONTEXT_TYPE_WORKER;
      return true;
    case RequestContextType::XML_HTTP_REQUEST:
      *out = content::REQUEST_CONTEXT_TYPE_XML_HTTP_REQUEST;
      return true;
    case RequestContextType::XSLT:
      *out = content::REQUEST_CONTEXT_TYPE_XSLT;
      return true;
  }

  return false;
}

ServiceWorkerFetchType
EnumTraits<ServiceWorkerFetchType, content::ServiceWorkerFetchType>::ToMojom(
    content::ServiceWorkerFetchType input) {
  switch (input) {
    case content::ServiceWorkerFetchType::FETCH:
      return ServiceWorkerFetchType::FETCH;
    case content::ServiceWorkerFetchType::FOREIGN_FETCH:
      return ServiceWorkerFetchType::FOREIGN_FETCH;
  }

  NOTREACHED();
  return ServiceWorkerFetchType::FETCH;
}

bool EnumTraits<ServiceWorkerFetchType, content::ServiceWorkerFetchType>::
    FromMojom(ServiceWorkerFetchType input,
              content::ServiceWorkerFetchType* out) {
  switch (input) {
    case ServiceWorkerFetchType::FETCH:
      *out = content::ServiceWorkerFetchType::FETCH;
      return true;
    case ServiceWorkerFetchType::FOREIGN_FETCH:
      *out = content::ServiceWorkerFetchType::FOREIGN_FETCH;
      return true;
  }

  return false;
}

std::map<std::string, std::string>
StructTraits<blink::mojom::fetch::RequestDataView,
             content::ServiceWorkerFetchRequest>::
    headers(const content::ServiceWorkerFetchRequest& request) {
  std::map<std::string, std::string> header_map;
  header_map.insert(request.headers.begin(), request.headers.end());
  return header_map;
}

bool StructTraits<blink::mojom::fetch::RequestDataView,
                  content::ServiceWorkerFetchRequest>::
    Read(blink::mojom::fetch::RequestDataView data,
         content::ServiceWorkerFetchRequest* out) {
  std::unordered_map<std::string, std::string> headers;
  base::Optional<std::string> blob_uuid;
  if (!data.ReadMode(&out->mode) ||
      !data.ReadRequestContextType(&out->request_context_type) ||
      !data.ReadFrameType(&out->frame_type) || !data.ReadUrl(&out->url) ||
      !data.ReadMethod(&out->method) || !data.ReadHeaders(&headers) ||
      !data.ReadBlobUuid(&blob_uuid) || !data.ReadReferrer(&out->referrer) ||
      !data.ReadCredentialsMode(&out->credentials_mode) ||
      !data.ReadRedirectMode(&out->redirect_mode) ||
      !data.ReadIntegrity(&out->integrity) ||
      !data.ReadClientId(&out->client_id) ||
      !data.ReadFetchType(&out->fetch_type)) {
    return false;
  }

  out->is_main_resource_load = data.is_main_resource_load();
  out->headers.insert(headers.begin(), headers.end());
  if (blob_uuid) {
    out->blob_uuid = blob_uuid.value();
    out->blob_size = data.blob_size();
  }
  storage::mojom::BlobPtr blob = data.TakeBlob<storage::mojom::BlobPtr>();
  if (blob)
    out->blob = base::MakeRefCounted<storage::BlobHandle>(std::move(blob));
  out->is_reload = data.is_reload();
  return true;
}

bool StructTraits<blink::mojom::fetch::ResponseDataView,
                  content::ServiceWorkerResponse>::
    Read(blink::mojom::fetch::ResponseDataView data,
         content::ServiceWorkerResponse* out) {
  std::unordered_map<std::string, std::string> headers;
  base::Optional<std::string> blob_uuid;
  if (!data.ReadUrlList(&out->url_list) ||
      !data.ReadStatusText(&out->status_text) ||
      !data.ReadResponseType(&out->response_type) ||
      !data.ReadHeaders(&headers) || !data.ReadBlobUuid(&blob_uuid) ||
      !data.ReadError(&out->error) ||
      !data.ReadResponseTime(&out->response_time) ||
      !data.ReadCacheStorageCacheName(&out->cache_storage_cache_name) ||
      !data.ReadCorsExposedHeaderNames(&out->cors_exposed_header_names)) {
    return false;
  }

  out->status_code = data.status_code();
  out->blob_size = data.blob_size();
#if 0
  // TODO(cmumford): Figure where this is set. Move to the out param?
  out->is_in_cache_storage = data.is_in_cache_storage();
#endif
  out->headers.insert(headers.begin(), headers.end());
  if (blob_uuid) {
    out->blob_uuid = blob_uuid.value();
    out->blob_size = data.blob_size();
  }

  return true;
}

std::map<std::string, std::string>
StructTraits<blink::mojom::fetch::ResponseDataView,
             content::ServiceWorkerResponse>::
    headers(const content::ServiceWorkerResponse& response) {
  std::map<std::string, std::string> headers;

  for (const auto& it : response.headers)
    headers[it.first] = it.second;

  return headers;
}

}  // namespace mojo
