// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerInstalledScriptsManager_h
#define WebServiceWorkerInstalledScriptsManager_h

#include "public/platform/WebCallbacks.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebVector.h"

namespace blink {

class WebServiceWorkerInstalledScriptsManager {
 public:
  using DataVector = WebVector<char>;
  struct RawScriptData {
    WebString encoding;
    WebVector<DataVector> script_text;
    WebVector<DataVector> meta_data;
  };

  virtual ~WebServiceWorkerInstalledScriptsManager() {}

  virtual std::unique_ptr<RawScriptData> GetRawScriptData(
      const WebURL& script_url) = 0;
};

}  // namespace blink

#endif  // WebServiceWorkerInstalledScriptsManager_h
