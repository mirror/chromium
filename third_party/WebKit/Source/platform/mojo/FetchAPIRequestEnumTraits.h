// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FetchAPIRequestEnumTraits_h
#define FetchAPIRequestEnumTraits_h

#include "public/platform/WebCommon.h"
#include "public/platform/modules/fetch/fetch_api_request.mojom.h"

namespace mojo {

template <>
struct BLINK_EXPORT EnumTraits<blink::mojom::ServiceWorkerResponseType,
                               blink::WebServiceWorkerResponseType> {
  static blink::mojom::ServiceWorkerResponseType ToMojom(
      blink::WebServiceWorkerResponseType input);

  static bool FromMojom(blink::mojom::ServiceWorkerResponseType input,
                        blink::WebServiceWorkerResponseType* out);
};

template <>
struct BLINK_EXPORT EnumTraits<blink::mojom::ResponseError,
                               blink::WebServiceWorkerResponseError> {
  static blink::mojom::ResponseError ToMojom(
      blink::WebServiceWorkerResponseError input);

  static bool FromMojom(blink::mojom::ResponseError input,
                        blink::WebServiceWorkerResponseError* out);
};

}  // namespace mojo

#endif  // FetchAPIRequestEnumTraits_h
