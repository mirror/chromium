
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/FetchAPIRequestEnumTraits.h"

#include "mojo/public/cpp/bindings/enum_traits.h"

namespace mojo {

using blink::mojom::ResponseError;
using blink::mojom::ServiceWorkerResponseType;

ServiceWorkerResponseType
EnumTraits<ServiceWorkerResponseType, blink::WebServiceWorkerResponseType>::
    ToMojom(blink::WebServiceWorkerResponseType input) {
  switch (input) {
    case blink::kWebServiceWorkerResponseTypeBasic:
      return ServiceWorkerResponseType::BASIC;
    case blink::kWebServiceWorkerResponseTypeCORS:
      return ServiceWorkerResponseType::CORS;
    case blink::kWebServiceWorkerResponseTypeDefault:
      return ServiceWorkerResponseType::DEFAULT;
    case blink::kWebServiceWorkerResponseTypeError:
      return ServiceWorkerResponseType::ERROR;
    case blink::kWebServiceWorkerResponseTypeOpaque:
      return ServiceWorkerResponseType::OPAQUE;
    case blink::kWebServiceWorkerResponseTypeOpaqueRedirect:
      return ServiceWorkerResponseType::OPAQUE_REDIRECT;
  }

  NOTREACHED();
  return ServiceWorkerResponseType::BASIC;
}

bool EnumTraits<ServiceWorkerResponseType,
                blink::WebServiceWorkerResponseType>::
    FromMojom(ServiceWorkerResponseType input,
              blink::WebServiceWorkerResponseType* out) {
  switch (input) {
    case ServiceWorkerResponseType::BASIC:
      *out = blink::kWebServiceWorkerResponseTypeBasic;
      return true;
    case ServiceWorkerResponseType::CORS:
      *out = blink::kWebServiceWorkerResponseTypeCORS;
      return true;
    case ServiceWorkerResponseType::DEFAULT:
      *out = blink::kWebServiceWorkerResponseTypeDefault;
      return true;
    case ServiceWorkerResponseType::ERROR:
      *out = blink::kWebServiceWorkerResponseTypeError;
      return true;
    case ServiceWorkerResponseType::OPAQUE:
      *out = blink::kWebServiceWorkerResponseTypeOpaque;
      return true;
    case ServiceWorkerResponseType::OPAQUE_REDIRECT:
      *out = blink::kWebServiceWorkerResponseTypeOpaqueRedirect;
      return true;
  }

  return false;
}

ResponseError
EnumTraits<ResponseError, blink::WebServiceWorkerResponseError>::ToMojom(
    blink::WebServiceWorkerResponseError input) {
  switch (input) {
    case blink::kWebServiceWorkerResponseErrorUnknown:
      return ResponseError::Unknown;
    case blink::kWebServiceWorkerResponseErrorPromiseRejected:
      return ResponseError::PromiseRejected;
    case blink::kWebServiceWorkerResponseErrorDefaultPrevented:
      return ResponseError::DefaultPrevented;
    case blink::kWebServiceWorkerResponseErrorNoV8Instance:
      return ResponseError::NoV8Instance;
    case blink::kWebServiceWorkerResponseErrorResponseTypeError:
      return ResponseError::ResponseTypeError;
    case blink::kWebServiceWorkerResponseErrorResponseTypeOpaque:
      return ResponseError::ResponseTypeOpaque;
    case blink::kWebServiceWorkerResponseErrorResponseTypeNotBasicOrDefault:
      return ResponseError::ResponseTypeNotBasicOrDefault;
    case blink::kWebServiceWorkerResponseErrorBodyUsed:
      return ResponseError::BodyUsed;
    case blink::
        kWebServiceWorkerResponseErrorResponseTypeOpaqueForClientRequest:
      return ResponseError::ResponseTypeOpaqueForClientRequest;
    case blink::kWebServiceWorkerResponseErrorResponseTypeOpaqueRedirect:
      return ResponseError::ResponseTypeOpaqueRedirect;
    case blink::kWebServiceWorkerResponseErrorBodyLocked:
      return ResponseError::BodyLocked;
    case blink::kWebServiceWorkerResponseErrorNoForeignFetchResponse:
      return ResponseError::NoForeignFetchResponse;
    case blink::kWebServiceWorkerResponseErrorForeignFetchHeadersWithoutOrigin:
      return ResponseError::ForeignFetchHeadersWithoutOrigin;
    case blink::kWebServiceWorkerResponseErrorForeignFetchMismatchedOrigin:
      return ResponseError::ForeignFetchMismatchedOrigin;
    case blink::
        kWebServiceWorkerResponseErrorRedirectedResponseForNotFollowRequest:
      return ResponseError::RedirectedResponseForNotFollowRequest;
  }

  NOTREACHED();
  return ResponseError::Unknown;
}

bool EnumTraits<ResponseError, blink::WebServiceWorkerResponseError>::FromMojom(
    ResponseError input,
    blink::WebServiceWorkerResponseError* out) {
  switch (input) {
    case ResponseError::Unknown:
      *out = blink::kWebServiceWorkerResponseErrorUnknown;
      return true;
    case ResponseError::PromiseRejected:
      *out = blink::kWebServiceWorkerResponseErrorPromiseRejected;
      return true;
    case ResponseError::DefaultPrevented:
      *out = blink::kWebServiceWorkerResponseErrorDefaultPrevented;
      return true;
    case ResponseError::NoV8Instance:
      *out = blink::kWebServiceWorkerResponseErrorNoV8Instance;
      return true;
    case ResponseError::ResponseTypeError:
      *out = blink::kWebServiceWorkerResponseErrorResponseTypeError;
      return true;
    case ResponseError::ResponseTypeOpaque:
      *out = blink::kWebServiceWorkerResponseErrorResponseTypeOpaque;
      return true;
    case ResponseError::ResponseTypeNotBasicOrDefault:
      *out = blink::kWebServiceWorkerResponseErrorResponseTypeNotBasicOrDefault;
      return true;
    case ResponseError::BodyUsed:
      *out = blink::kWebServiceWorkerResponseErrorBodyUsed;
      return true;
    case ResponseError::ResponseTypeOpaqueForClientRequest:
      *out = blink::
          kWebServiceWorkerResponseErrorResponseTypeOpaqueForClientRequest;
      return true;
    case ResponseError::ResponseTypeOpaqueRedirect:
      *out = blink::kWebServiceWorkerResponseErrorResponseTypeOpaqueRedirect;
      return true;
    case ResponseError::BodyLocked:
      *out = blink::kWebServiceWorkerResponseErrorBodyLocked;
      return true;
    case ResponseError::NoForeignFetchResponse:
      *out = blink::kWebServiceWorkerResponseErrorNoForeignFetchResponse;
      return true;
    case ResponseError::ForeignFetchHeadersWithoutOrigin:
      *out =
          blink::kWebServiceWorkerResponseErrorForeignFetchHeadersWithoutOrigin;
      return true;
    case ResponseError::ForeignFetchMismatchedOrigin:
      *out = blink::kWebServiceWorkerResponseErrorForeignFetchMismatchedOrigin;
      return true;
    case ResponseError::RedirectedResponseForNotFollowRequest:
      *out = blink::
          kWebServiceWorkerResponseErrorRedirectedResponseForNotFollowRequest;
      return true;
  }

  return false;
}

}  // namespace mojo
