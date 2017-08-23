
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_pages_ukm_reporter.h"

namespace offline_pages {

const char OfflinePagesUkmReporter::kRequestUkmEventName[] =
    "OfflinePages.SavePageRequested";
const char OfflinePagesUkmReporter::kForegroundUkmMetricName[] =
    "RequestedFromForeground";

}  // namespace offline_pages
