// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FetchAPIResponseStructTraits_h
#define FetchAPIResponseStructTraits_h

#include "platform/PlatformExport.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/fetch/fetch_api_response.mojom-blink.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerResponse.h"

namespace blink {
class KURL;
}

namespace mojo {

template <>
struct PLATFORM_EXPORT StructTraits<blink::mojom::FetchAPIResponseDataView,
                                    blink::WebServiceWorkerResponse> {
  static WTF::Vector<blink::KURL> url_list(
      const blink::WebServiceWorkerResponse&);

  static int status_code(const blink::WebServiceWorkerResponse& response) {
    return response.Status();
  }

  static WTF::String status_text(const blink::WebServiceWorkerResponse&);

  static network::mojom::FetchResponseType response_type(
      const blink::WebServiceWorkerResponse& response) {
    return response.ResponseType();
  }

  static WTF::HashMap<WTF::String, WTF::String> headers(
      const blink::WebServiceWorkerResponse&);

  static WTF::String blob_uuid(const blink::WebServiceWorkerResponse&);

  static uint64_t blob_size(const blink::WebServiceWorkerResponse& response) {
    return response.BlobSize();
  }

  static blink::mojom::ServiceWorkerResponseError error(
      const blink::WebServiceWorkerResponse& response) {
    return response.GetError();
  }

  static base::Time response_time(
      const blink::WebServiceWorkerResponse& response) {
    return response.ResponseTime();
  }

  static WTF::String cache_storage_cache_name(
      const blink::WebServiceWorkerResponse&);

  static WTF::Vector<WTF::String> cors_exposed_header_names(
      const blink::WebServiceWorkerResponse&);

  static bool Read(blink::mojom::FetchAPIResponseDataView,
                   blink::WebServiceWorkerResponse* output);
};

}  // namespace mojo

#endif  // FetchAPIResponseStructTraits_h
