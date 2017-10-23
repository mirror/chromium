// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_PROCESS_RESOURCE_COORDINATOR_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_PROCESS_RESOURCE_COORDINATOR_H_

#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/cpp/frame_resource_coordinator.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_export.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"

namespace service_manager {

class Connector;

}  // namespace service_manager

namespace resource_coordinator {

class SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT
    ProcessResourceCoordinator {
 public:
  ProcessResourceCoordinator(service_manager::Connector* connector);
  ~ProcessResourceCoordinator();

  void AddBinding(mojom::ProcessCoordinationUnitRequest request);
  void SetCPUUsage(double usage);
  void SetLaunchTime(base::Time launch_time);
  void SetPID(int64_t pid);

  void AddFrame(const FrameResourceCoordinator& frame);
  void RemoveFrame(const FrameResourceCoordinator& frame);

  CoordinationUnitID id() const { return cu_id_; }
  const mojom::ProcessCoordinationUnitPtr& service() const { return service_; }

 private:
  void ConnectToService(service_manager::Connector* connector,
                        const CoordinationUnitID& cu_id);
  void AddFrameByID(const CoordinationUnitID& cu_id);
  void RemoveFrameByID(const CoordinationUnitID& cu_id);

  THREAD_CHECKER(thread_checker_);

  mojom::ProcessCoordinationUnitPtr service_;
  CoordinationUnitID cu_id_;

  // The WeakPtrFactory should come last so the weak ptrs are invalidated
  // before the rest of the member variables.
  base::WeakPtrFactory<ProcessResourceCoordinator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProcessResourceCoordinator);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_PROCESS_RESOURCE_COORDINATOR_H_
