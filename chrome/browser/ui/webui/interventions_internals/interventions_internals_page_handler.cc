// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include "components/previews/core/previews_experiments.h"

namespace {

const char kAmpRedirectionPreviews[] = "ampRedirectionPreview";
const char kClientLoFi[] = "clientLoFi";
const char kOfflinePreviews[] = "offlinePreviews";

}  // namespace

InterventionsInternalsPageHandler::InterventionsInternalsPageHandler(
    mojom::InterventionsInternalsPageHandlerRequest request)
    : binding_(this, std::move(request)) {}

InterventionsInternalsPageHandler::~InterventionsInternalsPageHandler() {}

void InterventionsInternalsPageHandler::GetPreviewsEnabled(
    GetPreviewsEnabledCallback callback) {
  std::unordered_map<std::string, bool> statuses;

  statuses[kAmpRedirectionPreviews] =
      previews::params::IsAMPRedirectionPreviewEnabled();
  statuses[kClientLoFi] = previews::params::IsClientLoFiEnabled();
  statuses[kOfflinePreviews] = previews::params::IsOfflinePreviewsEnabled();

  std::move(callback).Run(statuses);
}
