// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerUpdateViaCache_h
#define WebServiceWorkerUpdateViaCache_h

namespace blink {

enum class WebServiceWorkerUpdateViaCache {
  kUnknown,
  kImports,
  kAll,
  kNone,
  kLast = kNone,
};

}  // namespace blink

#endif  // WebServiceWorkerUpdateViaCache_h
