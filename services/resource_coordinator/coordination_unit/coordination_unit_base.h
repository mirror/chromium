// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom.h"
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

  const CoordinationUnitID& id() const { return id_; }
  const base::ObserverList<CoordinationUnitGraphObserver>& observers() const {
    return observers_;
  }

 protected:
  static CoordinationUnitBase* GetCoordinationUnitByID(
      const CoordinationUnitID cu_id);

  const CoordinationUnitID id_;

 private:
  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  base::ObserverList<CoordinationUnitGraphObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitBase);
};

template <class CoordinationUnitImpl,
          class CoordinationUnitMojoInterface,
          class CoordinationUnitMojoRequest>
class CoordinationUnitInterface : public CoordinationUnitBase,
                                  public CoordinationUnitMojoInterface {
 public:
  static CoordinationUnitImpl* Create(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref) {
    auto* cu = CoordinationUnitBase::CreateCoordinationUnit(
        id, std::move(service_ref));
    return static_cast<CoordinationUnitImpl*>(cu);
  }

  CoordinationUnitInterface(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref)
      : CoordinationUnitBase(id, std::move(service_ref)), binding_(this) {}

  ~CoordinationUnitInterface() override = default;

  static const CoordinationUnitImpl* FromCoordinationUnitBase(
      const CoordinationUnitBase* cu) {
    DCHECK(cu->id().type == CoordinationUnitImpl::Type());
    return static_cast<const CoordinationUnitImpl*>(cu);
  }

  static CoordinationUnitImpl* FromCoordinationUnitBase(
      CoordinationUnitBase* cu) {
    DCHECK(cu->id().type == CoordinationUnitImpl::Type());
    return static_cast<CoordinationUnitImpl*>(cu);
  }

  void Bind(CoordinationUnitMojoRequest request) {
    binding_.Bind(std::move(request));
  }

  void GetID(const typename CoordinationUnitMojoInterface::GetIDCallback&
                 callback) override {
    callback.Run(id_);
  }
  void AddBinding(CoordinationUnitMojoRequest request) override {
    bindings_.AddBinding(this, std::move(request));
  }

  bool GetProperty(const mojom::PropertyType property_type,
                   int64_t* result) const {
    auto value_it = properties_.find(property_type);
    if (value_it != properties_.end()) {
      *result = value_it->second;
      return true;
    }
    return false;
  }
  void SetPropertyForTesting(int64_t value) {
    SetProperty(mojom::PropertyType::kTest, value);
  }

  const std::map<mojom::PropertyType, int64_t>& properties_for_testing() const {
    return properties_;
  }
  mojo::Binding<CoordinationUnitMojoInterface>& binding() { return binding_; }

 protected:
  static CoordinationUnitImpl* GetCoordinationUnitByID(
      const CoordinationUnitID cu_id) {
    DCHECK(cu_id.type == CoordinationUnitImpl::Type());
    auto* cu = CoordinationUnitBase::GetCoordinationUnitByID(cu_id);
    DCHECK(cu->id().type == CoordinationUnitImpl::Type());
    return static_cast<CoordinationUnitImpl*>(cu);
  }

  virtual bool HasAncestor(CoordinationUnitBase* ancestor) = 0;
  virtual bool HasDescendant(CoordinationUnitBase* descendant) = 0;

  // Propagate property change to relevant |CoordinationUnitBase| instances.
  virtual void PropagateProperty(mojom::PropertyType property_type,
                                 int64_t value) {}
  virtual void OnEventReceived(mojom::Event event) {
    for (auto& observer : observers())
      observer.OnEventReceived(this, event);
  }
  virtual void OnPropertyChanged(mojom::PropertyType property_type,
                                 int64_t value) {
    for (auto& observer : observers())
      observer.OnPropertyChanged(this, property_type, value);
  }

  void SendEvent(mojom::Event event) { OnEventReceived(event); }
  void SetProperty(mojom::PropertyType property_type, int64_t value) {
    // The |CoordinationUnitGraphObserver| API specification dictates that
    // the property is guarranteed to be set on the |CoordinationUnitBase|
    // and propagated to the appropriate associated |CoordianationUnitBase|
    // before |OnPropertyChanged| is invoked on all of the registered observers.
    properties_[property_type] = value;
    PropagateProperty(property_type, value);
    OnPropertyChanged(property_type, value);
  }

 private:
  std::map<mojom::PropertyType, int64_t> properties_;
  mojo::BindingSet<CoordinationUnitMojoInterface> bindings_;

  mojo::Binding<CoordinationUnitMojoInterface> binding_;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitInterface);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_BASE_H_
