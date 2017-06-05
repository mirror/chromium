// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_dispatcher_stub.h"

#include "base/memory/ptr_util.h"
#include "components/offline_pages/core/offline_page_item.h"

namespace offline_pages {

PrefetchDispatcherStub::PrefetchDispatcherStub() = default;
PrefetchDispatcherStub::~PrefetchDispatcherStub() = default;

void PrefetchDispatcherStub::AddCandidatePrefetchURLs(
    const std::vector<PrefetchURL>& suggested_urls) {
  latest_prefetch_urls = suggested_urls;
  new_suggestions_count++;
}

void PrefetchDispatcherStub::RemoveAllUnprocessedPrefetchURLs(
    const std::string& name_space) {
  latest_prefetch_urls.clear();
  remove_all_suggestions_count++;
}

void PrefetchDispatcherStub::RemovePrefetchURLsByClientId(
    const ClientId& client_id) {
  remove_by_client_id_count++;
  last_removed_client_id = base::MakeUnique<ClientId>(client_id);
}

void PrefetchDispatcherStub::BeginBackgroundTask(
    std::unique_ptr<ScopedBackgroundTask> task) {}

void PrefetchDispatcherStub::StopBackgroundTask(ScopedBackgroundTask* task) {}

void PrefetchDispatcherStub::SetService(PrefetchService* service) {}

}  // namespace offline_pages
