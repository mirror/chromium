// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/ServiceWorkerStructTraits.h"

#include "mojo/public/cpp/bindings/string_traits_wtf.h"

namespace mojo {

using blink::mojom::CacheError;

CacheError EnumTraits<CacheError, blink::WebServiceWorkerCacheError>::ToMojom(
    blink::WebServiceWorkerCacheError input) {
  switch (input) {
    case blink::kWebServiceWorkerCacheErrorNotImplemented:
      return CacheError::ErrorNotImplemented;
    case blink::kWebServiceWorkerCacheErrorNotFound:
      return CacheError::ErrorNotFound;
    case blink::kWebServiceWorkerCacheErrorExists:
      return CacheError::ErrorExists;
    case blink::kWebServiceWorkerCacheErrorQuotaExceeded:
      return CacheError::ErrorQuotaExceeded;
    case blink::kWebServiceWorkerCacheErrorCacheNameNotFound:
      return CacheError::ErrorCacheNameNotFound;
    case blink::kWebServiceWorkerCacheErrorTooLarge:
      return CacheError::ErrorQueryTooLarge;
    case blink::kWebServiceWorkerCacheErrorStorage:
      return CacheError::ErrorStorage;
  }

  NOTREACHED();
  return CacheError::ErrorQueryTooLarge;
}

bool EnumTraits<CacheError, blink::WebServiceWorkerCacheError>::FromMojom(
    CacheError input,
    blink::WebServiceWorkerCacheError* out) {
  switch (input) {
    case CacheError::Success:
      // TODO(cmumford): This struct trait should be going away in
      // Mojo-ification.
      *out = blink::kWebServiceWorkerCacheErrorNotImplemented;
      return true;
    case CacheError::ErrorNotImplemented:
      *out = blink::kWebServiceWorkerCacheErrorNotImplemented;
      return true;
    case CacheError::ErrorNotFound:
      *out = blink::kWebServiceWorkerCacheErrorNotFound;
      return true;
    case CacheError::ErrorExists:
      *out = blink::kWebServiceWorkerCacheErrorExists;
      return true;
    case CacheError::ErrorQuotaExceeded:
      *out = blink::kWebServiceWorkerCacheErrorQuotaExceeded;
      return true;
    case CacheError::ErrorCacheNameNotFound:
      *out = blink::kWebServiceWorkerCacheErrorCacheNameNotFound;
      return true;
    case CacheError::ErrorQueryTooLarge:
      *out = blink::kWebServiceWorkerCacheErrorTooLarge;
      return true;
    case CacheError::ErrorStorage:
      *out = blink::kWebServiceWorkerCacheErrorStorage;
      return true;
  }

  return false;
}

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
