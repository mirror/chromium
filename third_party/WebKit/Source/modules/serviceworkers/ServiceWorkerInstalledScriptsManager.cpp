// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/ServiceWorkerInstalledScriptsManager.h"

#include "core/html/parser/TextResourceDecoder.h"

namespace blink {

ServiceWorkerInstalledScriptsManager::ServiceWorkerInstalledScriptsManager(
    std::unique_ptr<WebServiceWorkerInstalledScriptsManager> manager)
    : manager_(std::move(manager)) {}

Optional<WorkerInstalledScriptsManager::ScriptData>
ServiceWorkerInstalledScriptsManager::GetScriptData(const KURL& script_url) {
  // This would be blocked on script streaming from the browser.
  std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
      raw_script_data = manager_->GetRawScriptData(script_url);
  if (!raw_script_data)
    return WTF::nullopt;
  std::unique_ptr<TextResourceDecoder> decoder;
  if (!raw_script_data->encoding.IsEmpty()) {
    decoder = TextResourceDecoder::Create(
        "text/javascript", WTF::TextEncoding(raw_script_data->encoding));
  } else {
    decoder = TextResourceDecoder::Create("text/javascript", UTF8Encoding());
  }
  ScriptData script_data;
  for (const auto& portion : raw_script_data->script_text) {
    script_data.source_text.append(
        decoder->Decode(portion.Data(), portion.size()));
  }
  script_data.meta_data = WTF::MakeUnique<Vector<char>>();
  for (const auto& portion : raw_script_data->meta_data)
    script_data.meta_data->Append(portion.Data(), portion.size());
  return Optional<ScriptData>(std::move(script_data));
}

}  // namespace blink
