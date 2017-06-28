// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/serviceworkers/ServiceWorkerInstalledScriptsManager.h"

#include "core/html/parser/TextResourceDecoder.h"

namespace blink {

ServiceWorkerInstalledScriptsManager::ServiceWorkerInstalledScriptsManager(
    std::unique_ptr<WebServiceWorkerInstalledScriptsManager> manager)
    : manager_(std::move(manager)) {}

bool ServiceWorkerInstalledScriptsManager::IsScriptInstalled(
    const KURL& script_url) const {
  return manager_->IsScriptInstalled(script_url);
}

Optional<WorkerInstalledScriptsManager::ScriptData>
ServiceWorkerInstalledScriptsManager::GetScriptData(const KURL& script_url) {
  // This would be blocked on script streaming from the browser.
  std::unique_ptr<WebServiceWorkerInstalledScriptsManager::RawScriptData>
      raw_script_data = manager_->GetRawScriptData(script_url);

  if (!raw_script_data)
    return WTF::nullopt;

  std::unique_ptr<TextResourceDecoder> decoder;
  if (!raw_script_data->Encoding().IsEmpty()) {
    decoder = TextResourceDecoder::Create(TextResourceDecoderOptions(
        TextResourceDecoderOptions::kPlainTextContent,
        WTF::TextEncoding(raw_script_data->Encoding())));
  } else {
    decoder = TextResourceDecoder::Create(TextResourceDecoderOptions(
        TextResourceDecoderOptions::kPlainTextContent));
  }

  WorkerInstalledScriptsManager::ScriptData script_data;
  for (const auto& portion : raw_script_data->ScriptText()) {
    script_data.source_text.append(
        decoder->Decode(portion.Data(), portion.size()));
  }
  if (raw_script_data->MetaData().size() > 0) {
    script_data.meta_data = WTF::MakeUnique<Vector<char>>();
    for (const auto& portion : raw_script_data->MetaData())
      script_data.meta_data->Append(portion.Data(), portion.size());
  }
  script_data.headers = std::move(raw_script_data->Headers());
  return Optional<ScriptData>(std::move(script_data));
}

}  // namespace blink
