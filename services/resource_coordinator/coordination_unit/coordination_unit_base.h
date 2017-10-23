// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/observer_list.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/interfaces/signals.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace resource_coordinator {

// CoordinationUnitBase implements shared functionality among different types of
// coordination units. A specific type of coordination unit will derive from
// this class and can override shared funtionality when needed.
class CoordinationUnitBase {
 public:
  static CoordinationUnitBase* CreateCoordinationUnit(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  static std::vector<CoordinationUnitBase*> GetCoordinationUnitsOfType(
      CoordinationUnitType cu_type);
  static void AssertNoActiveCoordinationUnits();
  static void ClearAllCoordinationUnits();

  // Recalculate property internally.
  virtual void RecalculateProperty(const mojom::PropertyType property_type) {}

  CoordinationUnitBase(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  virtual ~CoordinationUnitBase();

  void Destruct();
  void BeforeDestroyed();
  void AddObserver(CoordinationUnitGraphObserver* observer);
  void RemoveObserver(CoordinationUnitGraphObserver* observer);
  bool GetProperty(const mojom::PropertyType property_type,
                   int64_t* result) const;
  void SetPropertyForTesting(int64_t value) {
    SetProperty(mojom::PropertyType::kTest, value);
  }

  const CoordinationUnitID& id() const { return id_; }
  const base::ObserverList<CoordinationUnitGraphObserver>& observers() const {
    return observers_;
  }
  const std::map<mojom::PropertyType, int64_t>& properties_for_testing() const {
    return properties_;
  }

 protected:
  static CoordinationUnitBase* GetCoordinationUnitByID(
      const CoordinationUnitID cu_id);

  virtual bool HasAncestor(CoordinationUnitBase* ancestor) = 0;
  virtual bool HasDescendant(CoordinationUnitBase* descendant) = 0;

  // Propagate property change to relevant |CoordinationUnitBase| instances.
  virtual void PropagateProperty(mojom::PropertyType property_type,
                                 int64_t value) {}
  virtual void OnEventReceived(mojom::Event event);
  virtual void OnPropertyChanged(mojom::PropertyType property_type,
                                 int64_t value);

  void SendEvent(mojom::Event event);
  void SetProperty(mojom::PropertyType property_type, int64_t value);

  const CoordinationUnitID id_;

 private:
  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  std::map<mojom::PropertyType, int64_t> properties_;
  base::ObserverList<CoordinationUnitGraphObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitBase);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_
