// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/WorkletModuleScriptFetcher.h"

#include "bindings/core/v8/SourceLocation.h"
#include "core/inspector/ConsoleMessage.h"
#include "platform/CrossThreadFunctional.h"

namespace blink {

WorkletModuleScriptFetcher::WorkletModuleScriptFetcher(
    const FetchParameters& fetch_params,
    ResourceFetcher* fetcher,
    Modulator* modulator,
    ModuleScriptFetcher::Client* client,
    WorkletModuleResponsesMapProxy* module_responses_map_proxy)
    : ModuleScriptFetcher(fetch_params, fetcher, modulator, client),
      module_responses_map_proxy_(module_responses_map_proxy) {}

DEFINE_TRACE(WorkletModuleScriptFetcher) {
  visitor->Trace(module_responses_map_proxy_);
  ModuleScriptFetcher::Trace(visitor);
}

void WorkletModuleScriptFetcher::Fetch() {
  module_responses_map_proxy_->ReadEntry(GetFetchParams(), this);
}

void WorkletModuleScriptFetcher::OnRead(
    const ModuleScriptCreationParams& params) {
  // The context can be destroyed during cross-thread read operation.
  if (!HasValidContext()) {
    Finalize(WTF::nullopt);
    return;
  }
  Finalize(params);
}

void WorkletModuleScriptFetcher::OnFailed(ConsoleMessage* error_message) {
  if (error_message) {
    // Reconstruct the error message in the Oilpan's heap of the current thread.
    error_message = ConsoleMessage::CreateForRequest(
        kJSMessageSource, kErrorMessageLevel, error_message->Message(),
        error_message->Location()->Url(), error_message->RequestIdentifier());
    ReportErrorMessage(error_message);
  }
  Finalize(WTF::nullopt);
}

}  // namespace blink
