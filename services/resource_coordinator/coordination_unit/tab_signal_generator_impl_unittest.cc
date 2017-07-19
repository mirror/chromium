// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_signal_generator_impl.h"

#include <string>

#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_factory.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl_unittest_util.h"

namespace resource_coordinator {

const char kBackgroundAudioStartsUMA[] =
    "TabManager.Heuristics.BackgroundAudioStarts";

class TabSignalGeneratorTest : public CoordinationUnitImplTestBase {
 public:
  void SetUp() override {
    const_cast<base::TickClock*&>(
        tab_signal_generator_.metrics_collector_for_testing()->clock_) =
        &clock_;
    clock_.Advance(base::TimeDelta::FromSeconds(1000000));
  }

 protected:
  void AdvanceClock(base::TimeDelta delta) { clock_.Advance(delta); }

  base::HistogramTester histogram_tester_;
  TabSignalGeneratorImpl tab_signal_generator_;
  base::SimpleTestTickClock clock_;
};

TEST_F(TabSignalGeneratorTest, FromBackgroundedToFirstAudioStartsUMA) {
  coordination_unit_manager().RegisterObserver(
      base::WrapUnique(&tab_signal_generator_));
  CoordinationUnitID tab_cu_id(CoordinationUnitType::kWebContents,
                               std::string());
  CoordinationUnitID frame_cu_id(CoordinationUnitType::kFrame, std::string());

  std::unique_ptr<CoordinationUnitImpl> tab_coordination_unit =
      coordination_unit_factory::CreateCoordinationUnit(
          tab_cu_id, service_context_ref_factory()->CreateRef());
  std::unique_ptr<CoordinationUnitImpl> frame_coordination_unit =
      coordination_unit_factory::CreateCoordinationUnit(
          frame_cu_id, service_context_ref_factory()->CreateRef());
  coordination_unit_manager().OnCoordinationUnitCreated(
      tab_coordination_unit.get());
  coordination_unit_manager().OnCoordinationUnitCreated(
      frame_coordination_unit.get());

  tab_coordination_unit->AddChild(frame_coordination_unit->id());

  tab_coordination_unit->SetProperty(mojom::PropertyType::kVisible,
                                     base::MakeUnique<base::Value>(true));
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(true));
  // The tab is not backgrounded, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kBackgroundAudioStartsUMA, 0);
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(false));

  tab_coordination_unit->SetProperty(mojom::PropertyType::kVisible,
                                     base::MakeUnique<base::Value>(false));
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(true));
  // The tab was recently audible, thus no metrics recorded.
  histogram_tester_.ExpectTotalCount(kBackgroundAudioStartsUMA, 0);
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(false));

  AdvanceClock(base::TimeDelta::FromMinutes(1));
  tab_coordination_unit->SetProperty(mojom::PropertyType::kVisible,
                                     base::MakeUnique<base::Value>(true));
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(true));
  // The tab was not recently audible but it is not backgrounded, thus no
  // metrics recorded.
  histogram_tester_.ExpectTotalCount(kBackgroundAudioStartsUMA, 0);
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(false));

  tab_coordination_unit->SetProperty(mojom::PropertyType::kVisible,
                                     base::MakeUnique<base::Value>(false));
  AdvanceClock(base::TimeDelta::FromSeconds(61));
  frame_coordination_unit->SetProperty(mojom::PropertyType::kAudible,
                                       base::MakeUnique<base::Value>(true));
  // The tab was not recently audible and it is backgrounded, thus metrics
  // recorded.
  histogram_tester_.ExpectTotalCount(kBackgroundAudioStartsUMA, 1);
}

}  // namespace resource_coordinator
