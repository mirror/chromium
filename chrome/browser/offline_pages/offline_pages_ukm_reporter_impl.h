// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGES_UKM_REPORTER_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGES_UKM_REPORTER_IMPL_H_

#include "components/offline_pages/core/offline_pages_ukm_reporter.h"

#include "base/macros.h"

class GURL;

namespace offline_pages {

class OfflinePagesUkmReporterImpl : public OfflinePagesUkmReporter {
 public:
  OfflinePagesUkmReporterImpl() = default;
  ~OfflinePagesUkmReporterImpl() override = default;
  void ReportUrlOfflineRequest(const GURL& gurl, bool foreground) override;

  DISALLOW_COPY_AND_ASSIGN(OfflinePagesUkmReporterImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGES_UKM_REPORTER_IMPL_H_
