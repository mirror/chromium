// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PROCESS_COORDINATION_UNIT_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PROCESS_COORDINATION_UNIT_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

namespace resource_coordinator {

class FrameCoordinationUnitImpl;
class ProcessCoordinationUnitImpl;

class ProcessCoordinationUnitImpl : public CoordinationUnitBase,
                                    public mojom::ProcessCoordinationUnit {
 public:
  static ProcessCoordinationUnitImpl* Create(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);

  static const ProcessCoordinationUnitImpl* FromCoordinationUnitBase(
      const CoordinationUnitBase* cu);
  static ProcessCoordinationUnitImpl* FromCoordinationUnitBase(
      CoordinationUnitBase* cu);

  static CoordinationUnitType Type() { return CoordinationUnitType::kProcess; }

  ProcessCoordinationUnitImpl(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~ProcessCoordinationUnitImpl() override;

  void Bind(mojom::ProcessCoordinationUnitRequest request);

  // mojom::ProcessCoordinationUnit implementation.
  void GetID(
      const mojom::ProcessCoordinationUnit::GetIDCallback& callback) override;
  void AddBinding(mojom::ProcessCoordinationUnitRequest request) override;
  void AddFrame(const CoordinationUnitID& cu_id) override;
  void RemoveFrame(const CoordinationUnitID& cu_id) override;
  void SetCPUUsage(double cpu_usage) override;
  void SetExpectedTaskQueueingDuration(base::TimeDelta duration) override;
  void SetLaunchTime(base::Time launch_time) override;
  void SetPID(int64_t pid) override;

  std::set<PageCoordinationUnitImpl*> GetAssociatedPageCoordinationUnits()
      const;

  mojo::Binding<mojom::ProcessCoordinationUnit>& binding() { return binding_; }

 private:
  friend class FrameCoordinationUnitImpl;

  static ProcessCoordinationUnitImpl* GetCoordinationUnitByID(
      const CoordinationUnitID cu_id);

  // CoordinationUnitInterface implementation.
  bool HasAncestor(CoordinationUnitBase* ancestor) override;
  bool HasDescendant(CoordinationUnitBase* descendant) override;
  void PropagateProperty(mojom::PropertyType property_type,
                         int64_t value) override;

  bool AddFrame(FrameCoordinationUnitImpl* frame_cu);
  bool RemoveFrame(FrameCoordinationUnitImpl* frame_cu);

  std::set<FrameCoordinationUnitImpl*> frame_coordination_units_;
  mojo::BindingSet<mojom::ProcessCoordinationUnit> bindings_;
  mojo::Binding<mojom::ProcessCoordinationUnit> binding_;

  DISALLOW_COPY_AND_ASSIGN(ProcessCoordinationUnitImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_PROCESS_COORDINATION_UNIT_IMPL_H_
