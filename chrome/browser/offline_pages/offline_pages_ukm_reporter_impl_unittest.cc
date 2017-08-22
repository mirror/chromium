// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/offline_pages_ukm_reporter_impl.h"
#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const char kVisitedUrl[] = "http://m.en.wikipedia.org/wiki/Glider_(sailplane)";

// These must match offline_pages_ukm_reporter_impl.cc
const char kOfflinePageLoadUkmEventName[] = "OfflinePages";
const char kSavePageRequested[] = "SavePageRequested";
}  // namespace

namespace offline_pages {

class OfflinePagesUkmReporterImplTest : public testing::Test {
 public:
  ukm::TestUkmRecorder& test_recorder() { return test_recorder_; }

 private:
  ukm::TestAutoSetUkmRecorder test_recorder_;
};

TEST_F(OfflinePagesUkmReporterImplTest, RecordOfflinePageVisit) {
  OfflinePagesUkmReporterImpl reporter;
  GURL gurl(kVisitedUrl);

  reporter.ReportUrlOfflined(gurl, false);

  EXPECT_EQ(1U, test_recorder().sources_count());
  const ukm::UkmSource* found_source =
      test_recorder().GetSourceForUrl(kVisitedUrl);
  EXPECT_NE(nullptr, found_source);

  test_recorder().ExpectMetric(*found_source, kOfflinePageLoadUkmEventName,
                               kSavePageRequested, 0);
}

}  // namespace offline_pages
