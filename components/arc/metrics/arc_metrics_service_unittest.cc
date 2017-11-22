// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/metrics/arc_metrics_service.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#include "base/test/histogram_tester.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/test/fake_arc_session.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {
namespace {

// The event names the container sends to Chrome.
constexpr std::array<const char*, 11> kBootEvents{
    "boot_progress_start",
    "boot_progress_preload_start",
    "boot_progress_preload_end",
    "boot_progress_system_run",
    "boot_progress_pms_start",
    "boot_progress_pms_system_scan_start",
    "boot_progress_pms_data_scan_start",
    "boot_progress_pms_scan_end",
    "boot_progress_pms_ready",
    "boot_progress_ams_ready",
    "boot_progress_enable_screen"};

class ArcMetricsServiceTest : public testing::Test {
 public:
  ArcMetricsServiceTest() {
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::make_unique<chromeos::FakeSessionManagerClient>());
    GetSessionManagerClient()->set_arc_available(true);
  }

  ~ArcMetricsServiceTest() override { chromeos::DBusThreadManager::Shutdown(); }

  void SetUp() override {
    arc_service_manager_ = std::make_unique<ArcServiceManager>();
    prefs_ = std::make_unique<TestingPrefServiceSimple>();
    context_ = std::make_unique<content::TestBrowserContext>();
    user_prefs::UserPrefs::Set(context_.get(), prefs_.get());

    ArcMetricsService::GetFactory()->SetTestingFactoryAndUse(
        context_.get(),
        [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
          return std::make_unique<ArcMetricsService>(
              context, ArcServiceManager::Get()->arc_bridge_service());
        });
    metrics_service_ = ArcMetricsService::GetForBrowserContext(context_.get());
  }

  void TearDown() override {
    metrics_service_->Shutdown();

    context_.reset();
    prefs_.reset();
    arc_service_manager_.reset();
  }

  ArcMetricsService* metrics_service() { return metrics_service_; }

 protected:
  void SetArcStartTimeInMs(uint64_t arc_start_time_in_ms) {
    const base::TimeTicks arc_start_time =
        base::TimeDelta::FromMilliseconds(arc_start_time_in_ms) +
        base::TimeTicks();
    GetSessionManagerClient()->set_arc_start_time(arc_start_time);
  }

  std::vector<mojom::BootProgressEventPtr> GetBootProgressEvents(
      uint64_t start_in_ms,
      uint64_t step_in_ms) {
    std::vector<mojom::BootProgressEventPtr> events;
    for (size_t i = 0; i < kBootEvents.size(); ++i) {
      events.emplace_back(mojom::BootProgressEvent::New(
          kBootEvents[i], start_in_ms + (step_in_ms * i)));
    }
    return events;
  }

 private:
  chromeos::FakeSessionManagerClient* GetSessionManagerClient() {
    return static_cast<chromeos::FakeSessionManagerClient*>(
        chromeos::DBusThreadManager::Get()->GetSessionManagerClient());
  }

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<TestingPrefServiceSimple> prefs_;
  std::unique_ptr<content::TestBrowserContext> context_;

  ArcMetricsService* metrics_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ArcMetricsServiceTest);
};

// TODO(yusukes): Test ArcMetricsService::ParseProcessList() too.

// Tests that ReportBootProgress() actually records UMA stats.
TEST_F(ArcMetricsServiceTest, ReportBootProgress_FirstBoot) {
  // Start the full ARC container at t=10. Also set boot_progress_start to 10,
  // boot_progress_preload_start to 11, and so on.
  constexpr uint64_t kArcStartTimeMs = 10;
  SetArcStartTimeInMs(kArcStartTimeMs);
  std::vector<mojom::BootProgressEventPtr> events(
      GetBootProgressEvents(kArcStartTimeMs, 1 /* step_in_ms */));

  // Call ReportBootProgress() and then confirm that
  // Arc.boot_progress_start.FirstBoot is recorded with 0 (ms),
  // Arc.boot_progress_preload_start.FirstBoot is with 1 (ms), etc.
  base::HistogramTester tester;
  metrics_service()->ReportBootProgress(std::move(events),
                                        mojom::BootType::FIRST_BOOT);
  base::RunLoop().RunUntilIdle();
  for (size_t i = 0; i < kBootEvents.size(); ++i) {
    tester.ExpectUniqueSample(
        std::string("Arc.") + kBootEvents[i] + ".FirstBoot", i,
        1 /* count of the sample */);
    // Confirm that Arc.AndroidBootTime.FirstBoot is also recorded.
    if (i == kBootEvents.size() - 1)
      tester.ExpectUniqueSample("Arc.AndroidBootTime.FirstBoot", i, 1);
  }
}

// Does the same but with negative values and FIRST_BOOT_AFTER_UPDATE.
TEST_F(ArcMetricsServiceTest, ReportBootProgress_FirstBootAfterUpdate) {
  // Start the full ARC container at t=10. Also set boot_progress_start to 5,
  // boot_progress_preload_start to 7, and so on. This can actually happen
  // because the mini container can finish up to boot_progress_preload_end
  // before the full container is started.
  constexpr uint64_t kArcStartTimeMs = 10;
  SetArcStartTimeInMs(kArcStartTimeMs);
  std::vector<mojom::BootProgressEventPtr> events(
      GetBootProgressEvents(kArcStartTimeMs - 5, 2 /* step_in_ms */));

  // Call ReportBootProgress() and then confirm that
  // Arc.boot_progress_start.FirstBoot is recorded with 0 (ms),
  // Arc.boot_progress_preload_start.FirstBoot is with 0 (ms), etc. Unlike our
  // performance dashboard where negative performance numbers are treated as-is,
  // UMA treats them as zeros.
  base::HistogramTester tester;
  // This time, use mojom::BootType::FIRST_BOOT_AFTER_UPDATE.
  metrics_service()->ReportBootProgress(
      std::move(events), mojom::BootType::FIRST_BOOT_AFTER_UPDATE);
  base::RunLoop().RunUntilIdle();
  for (size_t i = 0; i < kBootEvents.size(); ++i) {
    const int expected = std::max<int>(0, i * 2 - 5);
    tester.ExpectUniqueSample(
        std::string("Arc.") + kBootEvents[i] + ".FirstBootAfterUpdate",
        expected, 1);
    if (i == kBootEvents.size() - 1) {
      tester.ExpectUniqueSample("Arc.AndroidBootTime.FirstBootAfterUpdate",
                                expected, 1);
    }
  }
}

// Does the same but with REGULAR_BOOT.
TEST_F(ArcMetricsServiceTest, ReportBootProgress_RegularBoot) {
  constexpr uint64_t kArcStartTimeMs = 10;
  SetArcStartTimeInMs(kArcStartTimeMs);
  std::vector<mojom::BootProgressEventPtr> events(
      GetBootProgressEvents(kArcStartTimeMs - 5, 2 /* step_in_ms */));

  base::HistogramTester tester;
  metrics_service()->ReportBootProgress(std::move(events),
                                        mojom::BootType::REGULAR_BOOT);
  base::RunLoop().RunUntilIdle();
  for (size_t i = 0; i < kBootEvents.size(); ++i) {
    const int expected = std::max<int>(0, i * 2 - 5);
    tester.ExpectUniqueSample(
        std::string("Arc.") + kBootEvents[i] + ".RegularBoot", expected, 1);
    if (i == kBootEvents.size() - 1)
      tester.ExpectUniqueSample("Arc.AndroidBootTime.RegularBoot", expected, 1);
  }
}

// Tests that no UMA is recorded when nothing is reported.
TEST_F(ArcMetricsServiceTest, ReportBootProgress_EmptyResults) {
  std::vector<mojom::BootProgressEventPtr> events;  // empty

  base::HistogramTester tester;
  metrics_service()->ReportBootProgress(std::move(events),
                                        mojom::BootType::FIRST_BOOT);
  base::RunLoop().RunUntilIdle();

  for (size_t i = 0; i < kBootEvents.size(); ++i) {
    tester.ExpectTotalCount(std::string("Arc.") + kBootEvents[i] + ".FirstBoot",
                            0);
  }
  tester.ExpectTotalCount("Arc.AndroidBootTime.FirstBoot", 0);
}

}  // namespace
}  // namespace arc
