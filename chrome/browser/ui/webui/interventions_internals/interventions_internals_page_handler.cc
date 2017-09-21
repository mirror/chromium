// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include "components/previews/core/previews_experiments.h"

InterventionsInternalsPageHandler::InterventionsInternalsPageHandler(
    mojom::InterventionsInternalsPageHandlerRequest request)
    : binding_(this, std::move(request)) {}

InterventionsInternalsPageHandler::~InterventionsInternalsPageHandler() {}

void InterventionsInternalsPageHandler::GetPreviewsEnabled(
    GetPreviewsEnabledCallback callback) {
  std::unordered_map<std::string, bool> statuses;

  statuses["AMPRedirectionPreview"] =
      previews::params::IsAMPRedirectionPreviewEnabled();
  statuses["ClientLoFi"] = previews::params::IsClientLoFiEnabled();
  statuses["OfflinePreviews"] = previews::params::IsOfflinePreviewsEnabled();

  std::move(callback).Run(statuses);
}
