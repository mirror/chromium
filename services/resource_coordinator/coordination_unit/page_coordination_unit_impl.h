// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_COORDINATION_UNIT_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_COORDINATION_UNIT_IMPL_H_

#include "base/macros.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

namespace resource_coordinator {

class FrameCoordinationUnitImpl;
class ProcessCoordinationUnitImpl;

class PageCoordinationUnitImpl : public CoordinationUnitBase,
                                 public mojom::PageCoordinationUnit {
 public:
  static PageCoordinationUnitImpl* Create(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  static const PageCoordinationUnitImpl* FromCoordinationUnitBase(
      const CoordinationUnitBase* cu);
  static PageCoordinationUnitImpl* FromCoordinationUnitBase(
      CoordinationUnitBase* cu);
  static CoordinationUnitType Type() { return CoordinationUnitType::kPage; }

  PageCoordinationUnitImpl(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~PageCoordinationUnitImpl() override;

  void Bind(mojom::PageCoordinationUnitRequest request);

  // mojom::PageCoordinationUnit implementation.
  void GetID(
      const mojom::PageCoordinationUnit::GetIDCallback& callback) override;
  void AddBinding(mojom::PageCoordinationUnitRequest request) override;
  void AddFrame(const CoordinationUnitID& cu_id) override;
  void RemoveFrame(const CoordinationUnitID& cu_id) override;
  void SetVisibility(bool visible) override;
  void SetUKMSourceId(int64_t ukm_source_id) override;
  void OnFaviconUpdated() override;
  void OnTitleUpdated() override;
  void OnMainFrameNavigationCommitted() override;

  // CoordinationUnitBase implementation.
  void RecalculateProperty(const mojom::PropertyType property_type) override;

  std::set<ProcessCoordinationUnitImpl*> GetAssociatedProcessCoordinationUnits()
      const;
  bool IsVisible() const;

  // Returns 0 if no navigation has happened, otherwise returns the time since
  // the last navigation commit.
  base::TimeDelta TimeSinceLastNavigation() const;

  // Returns the time since the last visibility change, it should always have a
  // value since we set the visibility property when we create a
  // PageCoordinationUnit.
  base::TimeDelta TimeSinceLastVisibilityChange() const;

  void SetClockForTest(std::unique_ptr<base::TickClock> test_clock);

  mojo::Binding<mojom::PageCoordinationUnit>& binding() { return binding_; }

  const std::set<FrameCoordinationUnitImpl*>&
  frame_coordination_units_for_testing() const {
    return frame_coordination_units_;
  }

 private:
  friend class FrameCoordinationUnitImpl;

  static PageCoordinationUnitImpl* GetCoordinationUnitByID(
      const CoordinationUnitID cu_id);

  // CoordinationUnitInterface implementation.
  bool HasAncestor(CoordinationUnitBase* ancestor) override;
  bool HasDescendant(CoordinationUnitBase* descendant) override;
  void OnEventReceived(mojom::Event event) override;
  void OnPropertyChanged(mojom::PropertyType property_type,
                         int64_t value) override;

  bool AddFrame(FrameCoordinationUnitImpl* frame_cu);
  bool RemoveFrame(FrameCoordinationUnitImpl* frame_cu);
  double CalculateCPUUsage();

  // Returns true for a valid value. Returns false otherwise.
  bool CalculateExpectedTaskQueueingDuration(int64_t* output);

  // Returns the main frame CU or nullptr if this page has no main frame.
  FrameCoordinationUnitImpl* GetMainFrameCoordinationUnit();

  std::set<FrameCoordinationUnitImpl*> frame_coordination_units_;

  std::unique_ptr<base::TickClock> clock_;
  base::TimeTicks visibility_change_time_;
  // Main frame navigation committed time.
  base::TimeTicks navigation_committed_time_;
  mojo::BindingSet<mojom::PageCoordinationUnit> bindings_;
  mojo::Binding<mojom::PageCoordinationUnit> binding_;

  DISALLOW_COPY_AND_ASSIGN(PageCoordinationUnitImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PAGE_COORDINATION_UNIT_IMPL_H_
