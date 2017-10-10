// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/chrome_metrics_service_client.h"

#include "base/files/file_path.h"
#include "base/process/process_handle.h"
#include "components/metrics/file_metrics_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

bool TestIsProcessRunning(base::ProcessId pid) {
  // Odd are running, even are not.
  return (pid & 1) == 1;
}

std::string CreateFilename(base::ProcessId pid) {
  return base::StringPrintf("foo-12345-%lX.pma", static_cast<long>(pid));
}

TEST(ChromeMetricsServiceClientTest, FilterFiles) {
  ChromeMetricsServiceClient::SetIsProcessRunningForTesting(
      &TestIsProcessRunning);

  base::ProcessId my_pid = base::GetCurrentProcId();

  EXPECT_EQ(metrics::FileMetricsProvider::FILTER_ACTIVE_THIS_PID,
            ChromeMetricsServiceClient::FilterBrowserMetricsFiles(
                base::FilePath::FromUTF8Unsafe(CreateFilename(my_pid))));
  EXPECT_EQ(
      metrics::FileMetricsProvider::FILTER_PROCESS_FILE,
      ChromeMetricsServiceClient::FilterBrowserMetricsFiles(
          base::FilePath::FromUTF8Unsafe(CreateFilename((my_pid & ~1) + 10))));
  EXPECT_EQ(
      metrics::FileMetricsProvider::FILTER_TRY_LATER,
      ChromeMetricsServiceClient::FilterBrowserMetricsFiles(
          base::FilePath::FromUTF8Unsafe(CreateFilename((my_pid & ~1) + 11))));
}

}  // namespace
