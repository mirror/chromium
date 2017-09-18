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
  bool enabled = previews::params::IsOfflinePreviewsEnabled() ||
                 previews::params::IsClientLoFiEnabled() ||
                 previews::params::IsAMPRedirectionPreviewEnabled();

  std::move(callback).Run(enabled);
}
