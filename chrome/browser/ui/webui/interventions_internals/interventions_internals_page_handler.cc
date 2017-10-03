// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include <unordered_map>
#include <vector>

#include "components/previews/core/previews_experiments.h"

namespace {

// Key for status mapping.
const char kAmpRedirectionPreviews[] = "ampPreviews";
const char kClientLoFiPreviews[] = "clientLoFiPreviews";
const char kOfflinePreviews[] = "offlinePreviews";

// Description for statuses.
const char kAmpRedirectionDescription[] = "AMP Previews";
const char kClientLoFiDescription[] = "Client LoFi Previews";
const char kOfflineDesciption[] = "Offline Previews";

}  // namespace

InterventionsInternalsPageHandler::InterventionsInternalsPageHandler(
    mojom::InterventionsInternalsPageHandlerRequest request,
    previews::PreviewsLogger* logger)
    : binding_(this, std::move(request)), logger_(logger) {}

InterventionsInternalsPageHandler::~InterventionsInternalsPageHandler() {}

void InterventionsInternalsPageHandler::GetPreviewsEnabled(
    GetPreviewsEnabledCallback callback) {
  std::unordered_map<std::string, mojom::PreviewsStatusPtr> statuses;

  auto amp_status = mojom::PreviewsStatus::New();
  amp_status->description = kAmpRedirectionDescription;
  amp_status->enabled = previews::params::IsAMPRedirectionPreviewEnabled();
  statuses[kAmpRedirectionPreviews] = std::move(amp_status);

  auto client_lofi_status = mojom::PreviewsStatus::New();
  client_lofi_status->description = kClientLoFiDescription;
  client_lofi_status->enabled = previews::params::IsClientLoFiEnabled();
  statuses[kClientLoFiPreviews] = std::move(client_lofi_status);

  auto offline_status = mojom::PreviewsStatus::New();
  offline_status->description = kOfflineDesciption;
  offline_status->enabled = previews::params::IsOfflinePreviewsEnabled();
  statuses[kOfflinePreviews] = std::move(offline_status);

  std::move(callback).Run(std::move(statuses));
}

void InterventionsInternalsPageHandler::GetMessageLogs(
    GetMessageLogsCallback callback) {
  std::vector<previews::PreviewsLogger::MessageLog> messages =
      logger_->log_messages();

  std::vector<mojom::MessageLogPtr> result;
  for (auto message : messages) {
    auto log = mojom::MessageLog::New();
    log->type = message.event_type;
    log->description = message.event_description;
    log->url = message.url.spec();

    result.push_back(std::move(log));
  }

  std::move(callback).Run(std::move(result));
}
