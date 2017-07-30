// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl_unittest_util.h"
#include "services/resource_coordinator/coordination_unit/mock_coordination_unit_graphs.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

namespace {

class WebContentsCoordinationUnitImplTest
    : public CoordinationUnitImplTestBase {};

}  // namespace

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabCPUUsageForSingleTabInSingleProcess) {
  MockSingleTabInSingleProcessCoordinationUnitGraph cu_graph;

  cu_graph.process->SetProperty(mojom::PropertyType::kCPUUsage,
                                base::MakeUnique<base::Value>(40.0));

  EXPECT_EQ(base::Value(40.0),
            cu_graph.tab->GetProperty(mojom::PropertyType::kCPUUsage));
}

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabCPUUsageForMultipleTabsInSingleProcess) {
  MockMultipleTabsInSingleProcessCoordinationUnitGraph cu_graph;

  cu_graph.process->SetProperty(mojom::PropertyType::kCPUUsage,
                                base::MakeUnique<base::Value>(40.0));

  EXPECT_EQ(base::Value(20.0),
            cu_graph.tab->GetProperty(mojom::PropertyType::kCPUUsage));
  EXPECT_EQ(base::Value(20.0),
            cu_graph.tab->GetProperty(mojom::PropertyType::kCPUUsage));
}

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabCPUUsageForSingleTabWithMultipleProcesses) {
  MockSingleTabWithMultipleProcessesCoordinationUnitGraph cu_graph;

  cu_graph.process->SetProperty(mojom::PropertyType::kCPUUsage,
                                base::MakeUnique<base::Value>(40.0));
  cu_graph.other_process->SetProperty(mojom::PropertyType::kCPUUsage,
                                      base::MakeUnique<base::Value>(30.0));

  EXPECT_EQ(base::Value(70.0),
            cu_graph.tab->GetProperty(mojom::PropertyType::kCPUUsage));
}

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabCPUUsageForMultipleTabsWithMultipleProcesses) {
  MockMultipleTabsWithMultipleProcessesCoordinationUnitGraph cu_graph;

  cu_graph.process->SetProperty(mojom::PropertyType::kCPUUsage,
                                base::MakeUnique<base::Value>(40.0));
  cu_graph.other_process->SetProperty(mojom::PropertyType::kCPUUsage,
                                      base::MakeUnique<base::Value>(30.0));

  EXPECT_EQ(base::Value(20.0),
            cu_graph.tab->GetProperty(mojom::PropertyType::kCPUUsage));
  EXPECT_EQ(base::Value(50.0),
            cu_graph.other_tab->GetProperty(mojom::PropertyType::kCPUUsage));
}

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabEQTForSingleTabInSingleProcess) {
  MockSingleTabInSingleProcessCoordinationUnitGraph cu_graph;

  cu_graph.process->SetProperty(
      mojom::PropertyType::kExpectedTaskQueueingDuration,
      base::MakeUnique<base::Value>(1.0));

  EXPECT_EQ(base::Value(1.0),
            cu_graph.tab->GetProperty(
                mojom::PropertyType::kExpectedTaskQueueingDuration));
}

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabEQTForMultipleTabsInSingleProcess) {
  MockMultipleTabsInSingleProcessCoordinationUnitGraph cu_graph;

  cu_graph.process->SetProperty(
      mojom::PropertyType::kExpectedTaskQueueingDuration,
      base::MakeUnique<base::Value>(1.0));

  EXPECT_EQ(base::Value(1.0),
            cu_graph.tab->GetProperty(
                mojom::PropertyType::kExpectedTaskQueueingDuration));
  EXPECT_EQ(base::Value(1.0),
            cu_graph.other_tab->GetProperty(
                mojom::PropertyType::kExpectedTaskQueueingDuration));
}

class MockCoordinationGraphObserver : public CoordinationUnitGraphObserver {
 public:
  // Overridden from CoordinationUnitGraphObserver.
  bool ShouldObserve(const CoordinationUnitImpl* coordination_unit) override {
    return coordination_unit->id().type == CoordinationUnitType::kWebContents;
  }
  void OnPropertyChanged(const CoordinationUnitImpl* coordination_unit,
                         const mojom::PropertyType property_type,
                         const base::Value& value) override {
    ++property_changed_count_;
  }

  size_t property_changed_count() const { return property_changed_count_; }

 private:
  size_t property_changed_count_ = 0;
};

TEST_F(WebContentsCoordinationUnitImplTest,
       CalculateTabEQTForSingleTabWithMultipleProcesses) {
  MockSingleTabWithMultipleProcessesCoordinationUnitGraph cu_graph;
  MockCoordinationGraphObserver mock_observer;
  cu_graph.tab->AddObserver(&mock_observer);

  cu_graph.process->SetProperty(
      mojom::PropertyType::kExpectedTaskQueueingDuration,
      base::MakeUnique<base::Value>(1.0));
  cu_graph.other_process->SetProperty(
      mojom::PropertyType::kExpectedTaskQueueingDuration,
      base::MakeUnique<base::Value>(10.0));

  // The |other_process| is not for the main frame so its EQT values does not
  // propagate to the tab.
  EXPECT_EQ(1u, mock_observer.property_changed_count());
  EXPECT_EQ(base::Value(1.0),
            cu_graph.tab->GetProperty(
                mojom::PropertyType::kExpectedTaskQueueingDuration));
}

}  // namespace resource_coordinator
