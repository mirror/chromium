// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"

namespace blink {

// static
std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManager::RawScriptData::Create(
    WebString encoding,
    WebVector<BytesChunk> script_text,
    WebVector<BytesChunk> meta_data) {
  return WTF::WrapUnique(
      new RawScriptData(std::move(encoding), std::move(script_text),
                        std::move(meta_data), true /* succeeded */));
}

// static
std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManager::RawScriptData::
    CreateFailureInstance() {
  return WTF::WrapUnique(
      new RawScriptData(WebString() /* encoding */, WebVector<BytesChunk>(),
                        WebVector<BytesChunk>(), false /* succeeded */));
}

WebServiceWorkerInstalledScriptsManager::RawScriptData::RawScriptData(
    WebString encoding,
    WebVector<BytesChunk> script_text,
    WebVector<BytesChunk> meta_data,
    bool succeeded)
    : succeeded_(succeeded),
      encoding_(std::move(encoding)),
      script_text_(std::move(script_text)),
      meta_data_(std::move(meta_data)),
      headers_(WTF::MakeUnique<CrossThreadHTTPHeaderMapData>()) {}

WebServiceWorkerInstalledScriptsManager::RawScriptData::~RawScriptData() =
    default;

void WebServiceWorkerInstalledScriptsManager::RawScriptData::AddHeader(
    const WebString& key,
    const WebString& value) {
  headers_->emplace_back(key, value);
}

}  // namespace blink
