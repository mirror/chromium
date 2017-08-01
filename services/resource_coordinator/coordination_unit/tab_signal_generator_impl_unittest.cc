// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/tab_signal_generator_impl.h"

#include "services/resource_coordinator/coordination_unit/coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_impl_unittest_util.h"
#include "services/resource_coordinator/coordination_unit/mock_coordination_unit_graphs.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

class TabSignalGeneratorImplTest : public CoordinationUnitImplTestBase {};

class MockTabSignalGeneratorImpl : public TabSignalGeneratorImpl {
 public:
  // Overridden from TabSignalGeneratorImpl.
  void OnPropertyChanged(const CoordinationUnitImpl* coordination_unit,
                         const mojom::PropertyType property_type,
                         const base::Value& value) override {
    if (coordination_unit->id().type == CoordinationUnitType::kWebContents &&
        property_type == mojom::PropertyType::kExpectedTaskQueueingDuration)
      ++eqt_change_count_;
  }

  size_t eqt_change_count() const { return eqt_change_count_; }

 private:
  size_t eqt_change_count_ = 0;
};

TEST_F(TabSignalGeneratorImplTest,
       CalculateTabEQTForSingleTabWithMultipleProcesses) {
  MockSingleTabWithMultipleProcessesCoordinationUnitGraph cu_graph;
  MockTabSignalGeneratorImpl mock_signal_generator;
  cu_graph.tab->AddObserver(&mock_signal_generator);

  cu_graph.process->SetProperty(
      mojom::PropertyType::kExpectedTaskQueueingDuration,
      base::MakeUnique<base::Value>(1.0));
  cu_graph.other_process->SetProperty(
      mojom::PropertyType::kExpectedTaskQueueingDuration,
      base::MakeUnique<base::Value>(10.0));

  // The |other_process| is not for the main frame so its EQT values does not
  // propagate to the tab.
  EXPECT_EQ(1u, mock_signal_generator.eqt_change_count());
  EXPECT_EQ(base::Value(1.0),
            cu_graph.tab->GetProperty(
                mojom::PropertyType::kExpectedTaskQueueingDuration));
}

}  // namespace resource_coordinator
