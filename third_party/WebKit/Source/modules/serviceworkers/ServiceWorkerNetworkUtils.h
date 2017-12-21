// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ServiceWorkerNetworkUtils_h
#define ServiceWorkerNetworkUtils_h

#include "modules/ModulesExport.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerRequest.h"

namespace blink {

class Request;
class ScriptState;

class MODULES_EXPORT ServiceWorkerNetworkUtils {
 public:
  static Request* ToRequest(ScriptState*, const WebServiceWorkerRequest&);
  static void PopulateWebServiceWorkerRequest(const Request&,
                                              WebServiceWorkerRequest&);
};

}  // namespace blink

#endif  // ServiceWorkerNetworkUtils
