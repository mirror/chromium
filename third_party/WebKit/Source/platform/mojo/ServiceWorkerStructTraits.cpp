// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/ServiceWorkerStructTraits.h"

#include "mojo/public/cpp/bindings/string_traits_wtf.h"

namespace mojo {

// static
WTF::String StructTraits<blink::mojom::QueryParamsDataView,
                         blink::WebServiceWorkerCache::QueryParams>::
    cache_name(const blink::WebServiceWorkerCache::QueryParams& query_params) {
  return query_params.cache_name;
}

// static
bool StructTraits<blink::mojom::QueryParamsDataView,
                  blink::WebServiceWorkerCache::QueryParams>::
    Read(blink::mojom::QueryParamsDataView data,
         blink::WebServiceWorkerCache::QueryParams* out) {
  base::Optional<WTF::String> cache_name;
  if (!data.ReadCacheName(&cache_name))
    return false;
  if (cache_name)
    out->cache_name = cache_name.value();

  out->ignore_search = data.ignore_search();
  out->ignore_method = data.ignore_method();
  out->ignore_vary = data.ignore_vary();

  return true;
}

}  // namespace mojo
