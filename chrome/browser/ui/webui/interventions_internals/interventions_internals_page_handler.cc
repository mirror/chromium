// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include <unordered_map>

#include "components/previews/core/previews_experiments.h"

namespace {

// Redirecting navigations to an AMP version of the page.
const char kAmpRedirectionPreviews[] = "AMP Previews";

// Showing a stored offline page.
const char kClientLoFiPreviews[] = "Client LoFi Previews";

// Replacing HTTPS images with small placeholders.
const char kOfflinePreviews[] = "Offline Previews";

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
  statuses[kClientLoFiPreviews] = previews::params::IsClientLoFiEnabled();
  statuses[kOfflinePreviews] = previews::params::IsOfflinePreviewsEnabled();

  std::move(callback).Run(statuses);
}
