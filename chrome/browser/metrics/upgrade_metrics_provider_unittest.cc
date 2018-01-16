// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/upgrade_metrics_provider.h"

#include "base/test/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/test/base/testing_browser_process.h"
#include "testing/gtest/include/gtest/gtest.h"

class UpgradeMetricsProviderTest : public testing::Test {
 public:
  UpgradeMetricsProviderTest() : upgrade_detector_(UpgradeDetector::Create()) {}

  void SetUp() override {
    TestingBrowserProcess::GetGlobal()->SetUpgradeDetector(
        upgrade_detector_.get());
  }
  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetUpgradeDetector(nullptr);
  }

  void TestHistogramLevel(
      UpgradeDetector::UpgradeNotificationAnnoyanceLevel level) {
    upgrade_detector_->NotifyUpgrade(level);
    base::HistogramTester histogram_tester;
    metrics_provider_.ProvideCurrentSessionData(nullptr);
    histogram_tester.ExpectUniqueSample("UpgradeDetector.NotificationStage",
                                        level, 1);
  }

 private:
  base::test::ScopedTaskEnvironment task_environment;
  std::unique_ptr<UpgradeDetector> upgrade_detector_;
  UpgradeMetricsProvider metrics_provider_;

  DISALLOW_COPY_AND_ASSIGN(UpgradeMetricsProviderTest);
};

TEST_F(UpgradeMetricsProviderTest, HistogramCheck) {
  TestHistogramLevel(UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  TestHistogramLevel(UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  TestHistogramLevel(UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  TestHistogramLevel(UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  TestHistogramLevel(UpgradeDetector::UPGRADE_ANNOYANCE_CRITICAL);
}
