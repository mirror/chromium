// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/metrics_collector.h"

#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "build/build_config.h"
#include "services/metrics/public/cpp/mojo_ukm_recorder.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/resource_coordinator_clock.h"
#include "services/service_manager/public/cpp/connector.h"

namespace resource_coordinator {

const base::TimeDelta kTestMetricsReportDelayTimeout =
    kMetricsReportDelayTimeout + base::TimeDelta::FromSeconds(1);
const base::TimeDelta kTestMaxAudioSlientTimeout =
    kMaxAudioSlientTimeout + base::TimeDelta::FromSeconds(1);
const char kTestMetricsServiceName[] = "test_metrics";

class TestMojoUkmRecorder : public ukm::MojoUkmRecorder {
 public:
  static std::unique_ptr<TestMojoUkmRecorder> Create(
      service_manager::Connector* connector) {
    ukm::mojom::UkmRecorderInterfacePtr interface;
    connector->BindInterface(kTestMetricsServiceName,
                             mojo::MakeRequest(&interface));
    return std::make_unique<TestMojoUkmRecorder>(std::move(interface));
  }

  explicit TestMojoUkmRecorder(ukm::mojom::UkmRecorderInterfacePtr interface)
      : ukm::MojoUkmRecorder(std::move(interface)) {}
  ~TestMojoUkmRecorder() override {}

  uint32_t entries_count() const { return entries_count_; }

  uint32_t sources_count() const { return sources_count_; }

  void UpdateSourceURL(ukm::SourceId source_id, const GURL& url) override {
    ++sources_count_;
    ukm::MojoUkmRecorder::UpdateSourceURL(source_id, url);
  }

  void AddEntry(ukm::mojom::UkmEntryPtr entry) override {
    ++entries_count_;
    ukm::MojoUkmRecorder::AddEntry(std::move(entry));
  }

 private:
  uint32_t entries_count_ = 0;
  uint32_t sources_count_ = 0;
};

// TODO(crbug.com/759905) Enable on Windows once this bug is fixed.
#if defined(OS_WIN)
#define MAYBE_MetricsCollectorTest DISABLED_MetricsCollectorTest
#else
#define MAYBE_MetricsCollectorTest MetricsCollectorTest
#endif
class MAYBE_MetricsCollectorTest : public CoordinationUnitTestHarness {
 public:
  MAYBE_MetricsCollectorTest() : CoordinationUnitTestHarness() {}

  void SetUp() override {
    MetricsCollector* metrics_collector = new MetricsCollector();
    ResourceCoordinatorClock::SetClockForTesting(
        std::make_unique<base::SimpleTestTickClock>());
    clock_ = static_cast<base::SimpleTestTickClock*>(
        ResourceCoordinatorClock::GetClockForTesting());

    // Sets a valid starting time.
    clock_->SetNowTicks(base::TimeTicks::Now());
    coordination_unit_manager().RegisterObserver(
        base::WrapUnique(metrics_collector));
  }

  void TearDown() override {
    clock_ = nullptr;
    ResourceCoordinatorClock::ResetClockForTesting();
  }

 protected:
  void AdvanceClock(base::TimeDelta delta) { clock_->Advance(delta); }

  base::HistogramTester histogram_tester_;
  base::SimpleTestTickClock* clock_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(MAYBE_MetricsCollectorTest);
};

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstAudioStartsUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  frame_cu->SetAudibility(true);
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);

  page_cu->SetVisibility(false);
  frame_cu->SetAudibility(true);
  // The page was recently audible, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);

  AdvanceClock(kTestMaxAudioSlientTimeout);
  page_cu->SetVisibility(true);
  frame_cu->SetAudibility(true);
  // The page was not recently audible but it is not backgrounded, thus no
  // metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);

  page_cu->SetVisibility(false);
  AdvanceClock(kTestMaxAudioSlientTimeout);
  frame_cu->SetAudibility(true);
  // The page was not recently audible and it is backgrounded, thus metrics
  // recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     1);
  frame_cu->SetAudibility(false);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  AdvanceClock(kTestMaxAudioSlientTimeout);
  frame_cu->SetAudibility(true);
  // The page becomes visible and then invisible again, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstAudioStartsUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());

  page_cu->AddFrame(frame_cu->id());

  page_cu->SetVisibility(false);
  page_cu->OnMainFrameNavigationCommitted();
  frame_cu->SetAudibility(true);
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     0);
  frame_cu->SetAudibility(false);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  frame_cu->SetAudibility(true);
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAudioStartsUMA,
                                     1);
}

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstTitleUpdatedUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  page_cu->OnTitleUpdated();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     0);

  page_cu->SetVisibility(false);
  page_cu->OnTitleUpdated();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     1);
  page_cu->OnTitleUpdated();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  page_cu->OnTitleUpdated();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstTitleUpdatedUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  page_cu->OnTitleUpdated();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  page_cu->OnTitleUpdated();
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstTitleUpdatedUMA,
                                     1);
}

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstAlertFiredUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  frame_cu->OnAlertFired();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     0);

  page_cu->SetVisibility(false);
  frame_cu->OnAlertFired();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     1);
  frame_cu->OnAlertFired();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  frame_cu->OnAlertFired();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstAlertFiredUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  frame_cu->OnAlertFired();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  frame_cu->OnAlertFired();
  histogram_tester_.ExpectTotalCount(kTabFromBackgroundedToFirstAlertFiredUMA,
                                     1);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstNonPersistentNotificationCreatedUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 0);

  page_cu->SetVisibility(false);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 1);
  frame_cu->OnNonPersistentNotificationCreated();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 2);
}

TEST_F(
    MAYBE_MetricsCollectorTest,
    FromBackgroundedToFirstNonPersistentNotificationCreatedUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  frame_cu->OnNonPersistentNotificationCreated();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  frame_cu->OnNonPersistentNotificationCreated();
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstNonPersistentNotificationCreatedUMA, 1);
}

TEST_F(MAYBE_MetricsCollectorTest, FromBackgroundedToFirstFaviconUpdatedUMA) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  AdvanceClock(kTestMetricsReportDelayTimeout);

  page_cu->SetVisibility(true);
  page_cu->OnFaviconUpdated();
  // The page is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 0);

  page_cu->SetVisibility(false);
  page_cu->OnFaviconUpdated();
  // The page is backgrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 1);
  page_cu->OnFaviconUpdated();
  // Metrics should only be recorded once per background period, thus metrics
  // not recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 1);

  page_cu->SetVisibility(true);
  page_cu->SetVisibility(false);
  page_cu->OnFaviconUpdated();
  // The page is backgrounded from foregrounded, thus metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 2);
}

TEST_F(MAYBE_MetricsCollectorTest,
       FromBackgroundedToFirstFaviconUpdatedUMA5MinutesTimeout) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());

  page_cu->OnMainFrameNavigationCommitted();
  page_cu->SetVisibility(false);
  page_cu->OnFaviconUpdated();
  // The page is within 5 minutes after main frame navigation was committed,
  // thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 0);
  AdvanceClock(kTestMetricsReportDelayTimeout);
  page_cu->OnFaviconUpdated();
  histogram_tester_.ExpectTotalCount(
      kTabFromBackgroundedToFirstFaviconUpdatedUMA, 1);
}

TEST_F(MAYBE_MetricsCollectorTest, ResponsivenessMetric) {
  auto page_cu = CreateCoordinationUnit<PageCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(page_cu.get());
  auto process_cu = CreateCoordinationUnit<ProcessCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(process_cu.get());

  auto frame_cu = CreateCoordinationUnit<FrameCoordinationUnitImpl>();
  coordination_unit_manager().OnCoordinationUnitCreated(frame_cu.get());
  page_cu->AddFrame(frame_cu->id());
  process_cu->AddFrame(frame_cu->id());

  service_manager::mojom::ConnectorRequest request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&request);
  std::unique_ptr<TestMojoUkmRecorder> mojo_ukm_recorder =
      TestMojoUkmRecorder::Create(connector.get());

  coordination_unit_manager().set_ukm_recorder(mojo_ukm_recorder.get());

  ukm::SourceId id = mojo_ukm_recorder->GetNewSourceID();
  mojo_ukm_recorder->UpdateSourceURL(id, GURL("https://google.com/foobar"));
  page_cu->SetUKMSourceId(id);
  page_cu->OnMainFrameNavigationCommitted();

  for (size_t count = 1; count < kFrequencyUkmEQTReported; ++count) {
    process_cu->SetExpectedTaskQueueingDuration(
        base::TimeDelta::FromMilliseconds(3));
    EXPECT_EQ(0U, mojo_ukm_recorder->entries_count());
    EXPECT_EQ(1U, mojo_ukm_recorder->sources_count());
  }
  process_cu->SetExpectedTaskQueueingDuration(
      base::TimeDelta::FromMilliseconds(3));
  EXPECT_EQ(1U, mojo_ukm_recorder->sources_count());
  EXPECT_EQ(1U, mojo_ukm_recorder->entries_count());
  for (size_t count = 1; count < kFrequencyUkmEQTReported; ++count) {
    process_cu->SetExpectedTaskQueueingDuration(
        base::TimeDelta::FromMilliseconds(3));
    EXPECT_EQ(1U, mojo_ukm_recorder->entries_count());
    EXPECT_EQ(1U, mojo_ukm_recorder->sources_count());
  }
  process_cu->SetExpectedTaskQueueingDuration(
      base::TimeDelta::FromMilliseconds(3));
  EXPECT_EQ(1U, mojo_ukm_recorder->sources_count());
  EXPECT_EQ(2U, mojo_ukm_recorder->entries_count());
}

}  // namespace resource_coordinator
