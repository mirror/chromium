// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerCacheError_h
#define WebServiceWorkerCacheError_h

namespace blink {

enum WebServiceWorkerCacheError {
  // TODO(cmumford): Delete after switch to Mojo.
  kWebServiceWorkerCacheErrorNotImplemented,
  kWebServiceWorkerCacheErrorNotFound,
  kWebServiceWorkerCacheErrorExists,
  kWebServiceWorkerCacheErrorQuotaExceeded,
  kWebServiceWorkerCacheErrorStorage,
  kWebServiceWorkerCacheErrorCacheNameNotFound,
  kWebServiceWorkerCacheErrorTooLarge,
  kWebServiceWorkerCacheErrorLast = kWebServiceWorkerCacheErrorTooLarge
};

}  // namespace blink

#endif  // WebServiceWorkerCacheError_h
