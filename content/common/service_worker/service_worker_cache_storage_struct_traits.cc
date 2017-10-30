// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_cache_storage_struct_traits.h"

#include "base/logging.h"
#include "content/public/common/referrer_struct_traits.h"

namespace mojo {

using blink::mojom::CacheError;
using blink::mojom::OperationType;

CacheError EnumTraits<CacheError, content::CacheStorageError>::ToMojom(
    content::CacheStorageError input) {
  switch (input) {
    case content::CACHE_STORAGE_ERROR_EXISTS:
      return CacheError::ErrorExists;
    case content::CACHE_STORAGE_ERROR_STORAGE:
      return CacheError::ErrorStorage;
    case content::CACHE_STORAGE_ERROR_NOT_FOUND:
      return CacheError::ErrorNotFound;
    case content::CACHE_STORAGE_ERROR_QUOTA_EXCEEDED:
      return CacheError::ErrorQuotaExceeded;
    case content::CACHE_STORAGE_ERROR_CACHE_NAME_NOT_FOUND:
      return CacheError::ErrorCacheNameNotFound;
    case content::CACHE_STORAGE_ERROR_QUERY_TOO_LARGE:
      return CacheError::ErrorQueryTooLarge;
    case content::CACHE_STORAGE_OK:
      return CacheError::Success;
  }

  NOTREACHED();
  return CacheError::ErrorNotImplemented;
}

OperationType
EnumTraits<OperationType, content::CacheStorageCacheOperationType>::ToMojom(
    content::CacheStorageCacheOperationType input) {
  switch (input) {
    case content::CACHE_STORAGE_CACHE_OPERATION_TYPE_UNDEFINED:
      return OperationType::Undefined;
    case content::CACHE_STORAGE_CACHE_OPERATION_TYPE_PUT:
      return OperationType::Put;
    case content::CACHE_STORAGE_CACHE_OPERATION_TYPE_DELETE:
      return OperationType::Delete;
  }

  NOTREACHED();
  return OperationType::Undefined;
}

bool EnumTraits<OperationType, content::CacheStorageCacheOperationType>::
    FromMojom(OperationType input,
              content::CacheStorageCacheOperationType* out) {
  switch (input) {
    case OperationType::Undefined:
      *out = content::CACHE_STORAGE_CACHE_OPERATION_TYPE_UNDEFINED;
      return true;
    case OperationType::Put:
      *out = content::CACHE_STORAGE_CACHE_OPERATION_TYPE_PUT;
      return true;
    case OperationType::Delete:
      *out = content::CACHE_STORAGE_CACHE_OPERATION_TYPE_DELETE;
      return true;
  }

  return false;
}

bool StructTraits<blink::mojom::QueryParamsDataView,
                  content::CacheStorageCacheQueryParams>::
    Read(blink::mojom::QueryParamsDataView data,
         content::CacheStorageCacheQueryParams* out) {
  base::Optional<base::string16> cache_name;
  if (!data.ReadCacheName(&cache_name))
    return false;
  if (cache_name)
    out->cache_name = base::NullableString16(cache_name);

  out->ignore_search = data.ignore_search();
  out->ignore_method = data.ignore_method();
  out->ignore_vary = data.ignore_vary();

  return true;
}

bool StructTraits<blink::mojom::BatchOperationDataView,
                  content::CacheStorageBatchOperation>::
    Read(blink::mojom::BatchOperationDataView data,
         content::CacheStorageBatchOperation* out) {
  if (!data.ReadRequest(&out->request))
    return false;
  if (!data.ReadResponse(&out->response))
    return false;
  if (!data.ReadMatchParams(&out->match_params))
    return false;
  if (!data.ReadOperationType(&out->operation_type))
    return false;

  return true;
}

}  // namespace mojo
