// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/modulescript/WorkletModuleScriptFetcher.h"

#include "platform/CrossThreadFunctional.h"

namespace blink {

WorkletModuleScriptFetcher::WorkletModuleScriptFetcher(
    const FetchParameters& fetch_params,
    ResourceFetcher* fetcher,
    Modulator* modulator,
    WorkletModuleResponsesMapProxy* module_responses_map_proxy)
    : ModuleScriptFetcher(fetch_params, fetcher, modulator),
      module_responses_map_proxy_(module_responses_map_proxy) {}

DEFINE_TRACE(WorkletModuleScriptFetcher) {
  visitor->Trace(module_responses_map_proxy_);
  ModuleScriptFetcher::Trace(visitor);
}

void WorkletModuleScriptFetcher::FetchInternal() {
  module_responses_map_proxy_->ReadEntry(GetRequestUrl(), this);
}

void WorkletModuleScriptFetcher::OnRead(
    const ModuleScriptCreationParams& params) {
  Finalize(params);
}

void WorkletModuleScriptFetcher::OnFetchNeeded() {
  // A target module hasn't been fetched yet. Fallback to the regular module
  // loading path. The module will be cached in NotifyFinished().
  was_fetched_via_network_ = true;
  ModuleScriptFetcher::FetchInternal();
}

void WorkletModuleScriptFetcher::OnFailed() {
  Finalize(WTF::nullopt);
}

void WorkletModuleScriptFetcher::Finalize(
    WTF::Optional<ModuleScriptCreationParams> params) {
  if (!params.has_value()) {
    module_responses_map_proxy_->InvalidateEntry(GetRequestUrl());
  } else if (was_fetched_via_network_) {
    module_responses_map_proxy_->UpdateEntry(GetRequestUrl(), *params);
  }
  ModuleScriptFetcher::Finalize(params);
}

}  // namespace blink
