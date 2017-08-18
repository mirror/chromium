// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_OFFLINE_PAGES_UKM_REPORTER_STUB_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_OFFLINE_PAGES_UKM_REPORTER_STUB_H_

#include "components/offline_pages/core/background/offline_pages_ukm_reporter.h"
#include "url/gurl.h"

namespace offline_pages {

class OfflinePagesUkmReporterStub : public OfflinePagesUkmReporter {
 public:
  void ReportUrlOfflined(const GURL& gurl) override;

  GURL GetLastOfflinedUrl() { return gurl_; }

 private:
  GURL gurl_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_OFFLINE_PAGES_UKM_REPORTER_STUB_H_
