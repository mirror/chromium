// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/generate_page_bundle_task.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "components/offline_pages/core/prefetch/prefetch_gcm_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"

namespace offline_pages {

namespace {

// Adds new prefetch item entries to the store using the URLs and client IDs
// from |prefetch_urls| and the client's |name_space|. Also cleans up entries in
// the Zombie state from the client's |name_space| except for the ones
// whose URL is contained in |prefetch_urls|.
// Returns the number of added prefecth items.
static int SelectURLsToPrefetch() {
  NOTIMPLEMENTED();
  return 1;
}

// TODO(fgorski): replace this with the SQL executor.
static void Execute(base::RepeatingCallback<int()> command_callback,
                    base::OnceCallback<void(int)> result_callback) {
  std::move(result_callback).Run(command_callback.Run());
}
}  // namespace

GeneratePageBundleTask::GeneratePageBundleTask(
    const std::vector<PrefetchURL>& prefetch_urls,
    PrefetchGCMHandler* gcm_handler,
    PrefetchNetworkRequestFactory* request_factory,
    const PrefetchRequestFinishedCallback& callback)
    : prefetch_urls_(prefetch_urls),
      gcm_handler_(gcm_handler),
      request_factory_(request_factory),
      callback_(callback),
      weak_factory_(this) {}

GeneratePageBundleTask::~GeneratePageBundleTask() {}

void GeneratePageBundleTask::Run() {
  Execute(base::BindRepeating(&SelectURLsToPrefetch),
          base::BindOnce(&GeneratePageBundleTask::StartGeneratePageBundle,
                         weak_factory_.GetWeakPtr()));
}

void GeneratePageBundleTask::StartGeneratePageBundle(int updated_entry_count) {
  if (prefetch_urls_.empty()) {
    TaskComplete();
    return;
  }

  gcm_handler_->GetGCMToken(base::Bind(
      &GeneratePageBundleTask::GotRegistrationId, weak_factory_.GetWeakPtr()));
}

void GeneratePageBundleTask::GotRegistrationId(
    const std::string& id,
    instance_id::InstanceID::Result result) {
  std::vector<std::string> url_strings;
  for (auto& prefetch_url : prefetch_urls_) {
    url_strings.push_back(prefetch_url.url.spec());
  }

  request_factory_->MakeGeneratePageBundleRequest(url_strings, id, callback_);
  TaskComplete();
}

}  // namespace offline_pages
