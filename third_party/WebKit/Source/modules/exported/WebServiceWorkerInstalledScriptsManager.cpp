// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace blink {

WebServiceWorkerInstalledScriptsManager::RawScriptData::RawScriptData(
    WebString encoding,
    WebVector<DataVector> script_text,
    WebVector<DataVector> meta_data)
    : encoding_(std::move(encoding)),
      script_text_(std::move(script_text)),
      meta_data_(std::move(meta_data)) {}

void WebServiceWorkerInstalledScriptsManager::RawScriptData::AddHeader(
    const WebString& key,
    const WebString& value) {
  headers_.Set(key, value);
}

}  // namespace blink
