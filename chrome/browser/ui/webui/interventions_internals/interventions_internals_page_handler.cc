// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include <unordered_map>

#include "base/strings/stringprintf.h"
#include "chrome/browser/previews/previews_service.h"
#include "chrome/browser/previews/previews_service_factory.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_ui_service.h"

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
    Profile* profile)
    : binding_(this, std::move(request)), profile_(profile) {}

InterventionsInternalsPageHandler::~InterventionsInternalsPageHandler() {
  if (profile_) {
    logger()->RemoveObserver(this);
  }
}

previews::PreviewsLogger* InterventionsInternalsPageHandler::logger() const {
  auto* logger = PreviewsServiceFactory::GetForProfile(profile_)
                     ->previews_ui_service()
                     ->previews_logger();
  DCHECK(logger);
  return logger;
}

void InterventionsInternalsPageHandler::SetClientPage(
    mojom::InterventionsInternalsPagePtr page) {
  page_ = std::move(page);
  DCHECK(page_);
  logger()->AddObserver(this);
}

void InterventionsInternalsPageHandler::OnNewMessageLogAdded(
    previews::PreviewsLogger::MessageLog message) {
  mojom::MessageLogPtr mojo_message_ptr(mojom::MessageLog::New());

  mojo_message_ptr->type = message.event_type;
  mojo_message_ptr->description = message.event_description;
  mojo_message_ptr->url = message.url.spec();
  mojo_message_ptr->time = message.time.ToJavaTime();

  DCHECK(page_);
  page_->LogNewMessage(std::move(mojo_message_ptr));
}

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
