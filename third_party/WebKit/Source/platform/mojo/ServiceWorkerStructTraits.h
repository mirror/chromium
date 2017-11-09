// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ServiceWorkerStructTraits_h
#define ServiceWorkerStructTraits_h

#include "platform/wtf/text/WTFString.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerCache.h"
#include "public/platform/modules/serviceworker/service_worker_cache_storage.mojom-blink.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::QueryParamsDataView,
                    blink::WebServiceWorkerCache::QueryParams> {
  static bool ignore_search(
      const blink::WebServiceWorkerCache::QueryParams& query_params) {
    return query_params.ignore_search;
  }

  static bool ignore_method(
      const blink::WebServiceWorkerCache::QueryParams& query_params) {
    return query_params.ignore_method;
  }

  static bool ignore_vary(
      const blink::WebServiceWorkerCache::QueryParams& query_params) {
    return query_params.ignore_vary;
  }

  static WTF::String cache_name(
      const blink::WebServiceWorkerCache::QueryParams&);

  static bool Read(blink::mojom::QueryParamsDataView,
                   blink::WebServiceWorkerCache::QueryParams* output);
};

}  // namespace mojo

#endif  // ServiceWorkerStructTraits_h
