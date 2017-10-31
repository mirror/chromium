// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cachestorage/CacheStorageError.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "modules/cachestorage/Cache.h"

using blink::mojom::ServiceWorkerCacheError;

namespace blink {

DOMException* CacheStorageError::CreateException(
    ServiceWorkerCacheError web_error) {
  switch (web_error) {
    case ServiceWorkerCacheError::kErrorNotImplemented:
      return DOMException::Create(kNotSupportedError,
                                  "Method is not implemented.");
    case ServiceWorkerCacheError::kErrorNotFound:
      return DOMException::Create(kNotFoundError, "Entry was not found.");
    case ServiceWorkerCacheError::kErrorExists:
      return DOMException::Create(kInvalidAccessError, "Entry already exists.");
    case ServiceWorkerCacheError::kErrorQuotaExceeded:
      return DOMException::Create(kQuotaExceededError, "Quota exceeded.");
    case ServiceWorkerCacheError::kErrorCacheNameNotFound:
      return DOMException::Create(kNotFoundError, "Cache was not found.");
    case ServiceWorkerCacheError::kErrorQueryTooLarge:
      return DOMException::Create(kAbortError, "Operation too large.");
    case ServiceWorkerCacheError::kErrorStorage:
      return DOMException::Create(kUnknownError, "Storage failure.");
    case ServiceWorkerCacheError::kSuccess:
      // This function should only be called with an error.
      break;
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace blink
