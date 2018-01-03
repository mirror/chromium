// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/FetchAPIResponseStructTraits.h"

#include "mojo/common/time_struct_traits.h"
#include "mojo/public/cpp/bindings/array_traits_wtf_vector.h"
#include "mojo/public/cpp/bindings/map_traits_wtf_hash_map.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "platform/mojo/KURLStructTraits.h"
#include "platform/mojo/ReferrerStructTraits.h"

namespace mojo {

// static
PLATFORM_EXPORT
bool StructTraits<blink::mojom::FetchAPIResponseDataView,
                  blink::WebServiceWorkerResponse>::
    Read(blink::mojom::FetchAPIResponseDataView data,
         blink::WebServiceWorkerResponse* out) {
  WTF::Vector<blink::KURL> urls;
  WTF::String status_text;
  network::mojom::FetchResponseType response_type;
  WTF::HashMap<WTF::String, WTF::String> headers;
  WTF::String blobUuid;
  blink::mojom::ServiceWorkerResponseError error;
  WTF::Time response_time;
  WTF::String cache_storage_cache_name;
  WTF::Vector<WTF::String> cors_exposed_header_names;
  if (!data.ReadUrlList(&urls) || !data.ReadStatusText(&status_text) ||
      !data.ReadResponseType(&response_type) || !data.ReadHeaders(&headers) ||
      !data.ReadBlobUuid(&blobUuid) || !data.ReadError(&error) ||
      !data.ReadResponseTime(&response_time) ||
      !data.ReadCacheStorageCacheName(&cache_storage_cache_name) ||
      !data.ReadCorsExposedHeaderNames(&cors_exposed_header_names)) {
    return false;
  }

  out->SetURLList(urls);
  out->SetStatus(data.status_code());
  out->SetStatusText(status_text);
  out->SetResponseType(response_type);
  for (const auto& pair : headers)
    out->SetHeader(pair.key, pair.value);
#if 0
  out->SetBlob(blobUuid, static_cast<long long>(data.blob_size()),
               data.TakeBlob<blink::mojom::blink::BlobPtr>().PassInterface());
#endif
  out->SetError(error);
  out->SetResponseTime(response_time);
  out->SetCacheStorageCacheName(cache_storage_cache_name);
  out->SetCorsExposedHeaderNames(cors_exposed_header_names);
  // Do I need this?
  // out->SetBlobDataHandle(RefPtr<BlobDataHandle>);

  return true;
}

// static
PLATFORM_EXPORT
WTF::Vector<blink::KURL> StructTraits<blink::mojom::FetchAPIResponseDataView,
                                      blink::WebServiceWorkerResponse>::
    url_list(const blink::WebServiceWorkerResponse& response) {
  const blink::WebVector<blink::WebURL>& response_urls = response.UrlList();
  WTF::Vector<blink::KURL> urls(response_urls.size());
  for (size_t i = 0; i < response_urls.size(); i++)
    urls[i] = response_urls[i];
  return urls;
}

PLATFORM_EXPORT
WTF::String StructTraits<blink::mojom::FetchAPIResponseDataView,
                         blink::WebServiceWorkerResponse>::
    status_text(const blink::WebServiceWorkerResponse& response) {
  return response.StatusText();
}

PLATFORM_EXPORT
WTF::HashMap<WTF::String, WTF::String>
StructTraits<blink::mojom::FetchAPIResponseDataView,
             blink::WebServiceWorkerResponse>::
    headers(const blink::WebServiceWorkerResponse& response) {
  WTF::HashMap<WTF::String, WTF::String> headers;

  for (const blink::WebString& key : response.GetHeaderKeys())
    headers.Set(key, response.GetHeader(key));

  return headers;
}

PLATFORM_EXPORT
WTF::String StructTraits<blink::mojom::FetchAPIResponseDataView,
                         blink::WebServiceWorkerResponse>::
    blob_uuid(const blink::WebServiceWorkerResponse& response) {
  return response.BlobUUID();
}

PLATFORM_EXPORT
WTF::String StructTraits<blink::mojom::FetchAPIResponseDataView,
                         blink::WebServiceWorkerResponse>::
    cache_storage_cache_name(const blink::WebServiceWorkerResponse& response) {
  return response.CacheStorageCacheName();
}

PLATFORM_EXPORT
WTF::Vector<WTF::String> StructTraits<blink::mojom::FetchAPIResponseDataView,
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
