// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FetchAPIRequestStructTraits_h
#define FetchAPIRequestStructTraits_h

#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebURLRequest.h"
#include "public/platform/modules/fetch/fetch_api_request.mojom-blink.h"

namespace blink {
class KURL;
}

namespace mojo {

template <>
struct EnumTraits<::blink::mojom::fetch::CredentialsMode,
                  ::blink::WebURLRequest::FetchCredentialsMode> {
  static ::blink::mojom::fetch::CredentialsMode ToMojom(
      ::blink::WebURLRequest::FetchCredentialsMode input);

  static bool FromMojom(::blink::mojom::fetch::CredentialsMode input,
                        ::blink::WebURLRequest::FetchCredentialsMode* out);
};

template <>
struct EnumTraits<blink::mojom::fetch::RedirectMode,
                  ::blink::WebURLRequest::FetchRedirectMode> {
  static blink::mojom::fetch::RedirectMode ToMojom(
      ::blink::WebURLRequest::FetchRedirectMode input);

  static bool FromMojom(blink::mojom::fetch::RedirectMode input,
                        ::blink::WebURLRequest::FetchRedirectMode* out);
};

template <>
struct EnumTraits<blink::mojom::fetch::RequestMode,
                  ::blink::WebURLRequest::FetchRequestMode> {
  static blink::mojom::fetch::RequestMode ToMojom(
      ::blink::WebURLRequest::FetchRequestMode input);

  static bool FromMojom(blink::mojom::fetch::RequestMode input,
                        ::blink::WebURLRequest::FetchRequestMode* out);
};

template <>
struct EnumTraits<::blink::mojom::fetch::RequestContextFrameType,
                  ::blink::WebURLRequest::FrameType> {
  static ::blink::mojom::fetch::RequestContextFrameType ToMojom(
      ::blink::WebURLRequest::FrameType input);

  static bool FromMojom(::blink::mojom::fetch::RequestContextFrameType input,
                        ::blink::WebURLRequest::FrameType* out);
};

template <>
struct EnumTraits<::blink::mojom::fetch::RequestContextType,
                  ::blink::WebURLRequest::RequestContext> {
  static ::blink::mojom::fetch::RequestContextType ToMojom(
      ::blink::WebURLRequest::RequestContext input);

  static bool FromMojom(::blink::mojom::fetch::RequestContextType input,
                        ::blink::WebURLRequest::RequestContext* out);
};

template <>
struct StructTraits<::blink::mojom::fetch::RequestDataView,
                    ::blink::WebServiceWorkerRequest> {
  static ::blink::WebURLRequest::FetchRequestMode mode(
      const ::blink::WebServiceWorkerRequest& request) {
    return request.Mode();
  }

  static bool is_main_resource_load(
      const ::blink::WebServiceWorkerRequest& request) {
    return request.IsMainResourceLoad();
  }

  static ::blink::WebURLRequest::RequestContext request_context_type(
      const ::blink::WebServiceWorkerRequest& request) {
    return request.GetRequestContext();
  }

  static ::blink::WebURLRequest::FrameType frame_type(
      const ::blink::WebServiceWorkerRequest& request) {
    return request.GetFrameType();
  }

  static ::blink::KURL url(const ::blink::WebServiceWorkerRequest&);

  static WTF::String method(const ::blink::WebServiceWorkerRequest&);

  static WTF::HashMap<WTF::String, WTF::String> headers(
      const ::blink::WebServiceWorkerRequest&);

  static WTF::String blob_uuid(const ::blink::WebServiceWorkerRequest&);

  static uint64_t blob_size(const ::blink::WebServiceWorkerRequest&);

  static storage::mojom::blink::BlobPtr blob(
      const ::blink::WebServiceWorkerRequest&);

  static const ::blink::Referrer& referrer(
      const ::blink::WebServiceWorkerRequest&);

  static ::blink::WebURLRequest::FetchCredentialsMode credentials_mode(
      const ::blink::WebServiceWorkerRequest& request) {
    return request.CredentialsMode();
  }

  static ::blink::WebURLRequest::FetchRedirectMode redirect_mode(
      const ::blink::WebServiceWorkerRequest& request) {
    return request.RedirectMode();
  }

  static WTF::String integrity(const ::blink::WebServiceWorkerRequest&);
  static WTF::String client_id(const ::blink::WebServiceWorkerRequest&);

  static bool is_reload(const ::blink::WebServiceWorkerRequest& request) {
    return request.IsReload();
  }

  // The |fetch_type| is not used by ::blink yet.
  static ::blink::mojom::fetch::ServiceWorkerFetchType fetch_type(
      const ::blink::WebServiceWorkerRequest& request) {
    return ::blink::mojom::fetch::ServiceWorkerFetchType::FETCH;
  }

  static bool Read(::blink::mojom::fetch::RequestDataView,
                   ::blink::WebServiceWorkerRequest* output);
};

template <>
struct StructTraits<blink::mojom::fetch::ResponseDataView,
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

  static blink::mojom::fetch::ResponseError error(
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

  static bool Read(blink::mojom::fetch::ResponseDataView,
                   blink::WebServiceWorkerResponse* output);
};

}  // namespace mojo

#endif  // FetchAPIRequestStructTraits_h
