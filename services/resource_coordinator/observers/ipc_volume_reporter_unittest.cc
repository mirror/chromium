// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/ipc_volume_reporter.h"

#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_test_harness.h"
#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/mock_coordination_unit_graphs.h"
#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/resource_coordinator_clock.h"

namespace resource_coordinator {

class IPCVolumeReporterTest : public CoordinationUnitTestHarness {
 public:
  IPCVolumeReporterTest() : CoordinationUnitTestHarness() {}

  void SetUp() override {
    ResourceCoordinatorClock::SetClockForTesting(
        std::make_unique<base::SimpleTestTickClock>());
    clock_ = static_cast<base::SimpleTestTickClock*>(
        ResourceCoordinatorClock::GetClockForTesting());
    coordination_unit_manager().RegisterObserver(
        std::make_unique<IPCVolumeReporter>());
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
  DISALLOW_COPY_AND_ASSIGN(IPCVolumeReporterTest);
};

TEST_F(IPCVolumeReporterTest, Basic) {
  MockSinglePageInSingleProcessCoordinationUnitGraph cu_graph;
  coordination_unit_manager().OnCoordinationUnitCreated(cu_graph.process.get());
  coordination_unit_manager().OnCoordinationUnitCreated(cu_graph.page.get());
  coordination_unit_manager().OnCoordinationUnitCreated(cu_graph.frame.get());

  cu_graph.process->SetCPUUsage(1.0);
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.process->SetExpectedTaskQueueingDuration(
      base::TimeDelta::FromMilliseconds(1));
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.process->SetLaunchTime(base::Time());
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.process->SetMainThreadTaskLoadIsLow(true);
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.process->SetPID(1);
  AdvanceClock(base::TimeDelta::FromSeconds(61));

  cu_graph.page->SetVisibility(true);
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.page->SetUKMSourceId(1);
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.page->OnFaviconUpdated();
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.page->OnTitleUpdated();
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.page->OnMainFrameNavigationCommitted();
  AdvanceClock(base::TimeDelta::FromSeconds(61));

  cu_graph.frame->SetAudibility(true);
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.frame->SetNetworkAlmostIdle(true);
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.frame->OnAlertFired();
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  cu_graph.frame->OnNonPersistentNotificationCreated();

  histogram_tester_.ExpectTotalCount("ResourceCoordinator.IPCPerMinute.Frame",
                                     13);
  histogram_tester_.ExpectTotalCount("ResourceCoordinator.IPCPerMinute.Page",
                                     13);
  histogram_tester_.ExpectTotalCount("ResourceCoordinator.IPCPerMinute.Process",
                                     13);
};

}  // namespace resource_coordinator
