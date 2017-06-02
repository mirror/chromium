// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_IMPL_H_

#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

#include <vector>
#include "base/callback.h"

#include "base/optional.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit_provider.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace resource_coordinator {

// Collection to manage CoordinationUnitEvent listeners.
// It is naive prototype with lots of room for improvement.
template <typename Listener, typename Filter>
class CoordinationUnitEventListenerList {
 public:
  CoordinationUnitEventListenerList() = default;
  ~CoordinationUnitEventListenerList() = default;

  // callbacks paired with this kWildcardFilter will always be invoked
  static const Filter kWildcardFilter = static_cast<Filter>(0);

  void AddListener(Listener listener, Filter filter) {
    listeners_.push_back(std::make_pair(filter, listener));
  }

  void AddListener(Listener listener) {
    AddListener(listener, kWildcardFilter);
  }

  // TODO(matthalp) add iterator support to replace this call and
  // avoid excessive copying that currently occurs
  std::vector<Listener> GetListeners(Filter filter) {
    std::vector<Listener> listeners;

    for (auto& listener_info : listeners_) {
      if (listener_info.first == filter ||
          listener_info.first == kWildcardFilter) {
        listeners.push_back(listener_info.second);
      }
    }

    return listeners;
  }

  std::vector<Listener> GetListeners() { return GetListeners(kWildcardFilter); }

 private:
  std::vector<std::pair<Filter, Listener>> listeners_;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitEventListenerList);
};

class CoordinationUnitImpl : public mojom::CoordinationUnit {
  typedef base::Callback<void(CoordinationUnitImpl* coordination_unit,
                              CoordinationUnitImpl* child_coordination_unit)>
      AddChildEventListener;
  typedef base::Callback<void(CoordinationUnitImpl* coordination_unit,
                              CoordinationUnitImpl* parent_coordination_unit)>
      AddParentEventListener;
  typedef base::Callback<void(
      CoordinationUnitImpl* coordination_unit,
      CoordinationUnitImpl* removed_child_coordination_unit)>
      RemoveChildEventListener;
  typedef base::Callback<void(
      CoordinationUnitImpl* coordination_unit,
      CoordinationUnitImpl* removed_parent_coordination_unit)>
      RemoveParentEventListener;
  typedef base::Callback<void(CoordinationUnitImpl* coordination_unit,
                              mojom::PropertyType property)>
      PropertyChangedEventListener;
  typedef base::Callback<void(CoordinationUnitImpl* coordination_unit)>
      WillBeDestroyedEventListener;

 public:
  CoordinationUnitImpl(
      const CoordinationUnitID& id,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~CoordinationUnitImpl() override;

  // Overridden from mojom::CoordinationUnit:
  void SendEvent(mojom::EventPtr event) override;
  void GetID(const GetIDCallback& callback) override;
  void Duplicate(mojom::CoordinationUnitRequest request) override;
  void AddChild(const CoordinationUnitID& child_id) override;
  void SetCoordinationPolicyCallback(
      mojom::CoordinationPolicyCallbackPtr callback) override;
  void SetProperty(mojom::PropertyPtr property) override;

  const CoordinationUnitID& id() const { return id_; }
  const std::set<CoordinationUnitImpl*>& children() const { return children_; }
  const std::set<CoordinationUnitImpl*>& parents() const { return parents_; }
  std::unordered_map<mojom::PropertyType, base::Value>& property_store() {
    return property_store_;
  }

  static const double kCPUUsageMinimumForTesting;
  static const double kCPUUsageUnmeasuredForTesting;
  virtual double GetCPUUsageForTesting();

  void ClearProperty(mojom::PropertyType property);
  base::Value GetProperty(mojom::PropertyType property);
  void SetProperty(mojom::PropertyType property, base::Value value);

  // allow the ability to be notify an instance before its destructor is called
  void WillBeDestroyed();

  CoordinationUnitEventListenerList<AddChildEventListener,
                                    CoordinationUnitType>&
  add_child_event_listeners() {
    return add_child_event_listeners_;
  }
  CoordinationUnitEventListenerList<AddParentEventListener,
                                    CoordinationUnitType>&
  add_parent_event_listeners() {
    return add_parent_event_listeners_;
  }
  CoordinationUnitEventListenerList<RemoveChildEventListener,
                                    CoordinationUnitType>&
  remove_child_event_listeners() {
    return remove_child_event_listeners_;
  }
  CoordinationUnitEventListenerList<RemoveParentEventListener,
                                    CoordinationUnitType>&
  remove_parent_event_listeners() {
    return remove_parent_event_listeners_;
  }
  CoordinationUnitEventListenerList<PropertyChangedEventListener,
                                    mojom::PropertyType>&
  storage_property_changed_event_listeners() {
    return storage_property_changed_event_listeners_;
  }
  CoordinationUnitEventListenerList<WillBeDestroyedEventListener,
                                    CoordinationUnitType>&
  will_be_destroyed_event_listeners() {
    return will_be_destroyed_event_listeners_;
  }

 protected:
  const CoordinationUnitID id_;
  std::set<CoordinationUnitImpl*> children_;
  std::set<CoordinationUnitImpl*> parents_;

 private:
  bool AddChild(CoordinationUnitImpl* child);
  void RemoveChild(CoordinationUnitImpl* child);
  void AddParent(CoordinationUnitImpl* parent);
  void RemoveParent(CoordinationUnitImpl* parent);
  bool HasParent(CoordinationUnitImpl* unit);
  bool HasChild(CoordinationUnitImpl* unit);

  void RecalcCoordinationPolicy();
  void UnregisterCoordinationPolicyCallback();

  std::unordered_map<mojom::PropertyType, base::Value> property_store_;

  enum StateFlags : uint8_t {
    kTestState,
    kTabVisible,
    kAudioPlaying,
    kNumStateFlags
  };
  bool SelfOrParentHasFlagSet(StateFlags state);

  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  mojo::BindingSet<mojom::CoordinationUnit> bindings_;

  mojom::CoordinationPolicyCallbackPtr policy_callback_;
  mojom::CoordinationPolicyPtr current_policy_;

  base::Optional<bool> state_flags_[kNumStateFlags];

  CoordinationUnitEventListenerList<AddChildEventListener, CoordinationUnitType>
      add_child_event_listeners_;
  CoordinationUnitEventListenerList<AddParentEventListener,
                                    CoordinationUnitType>
      add_parent_event_listeners_;
  CoordinationUnitEventListenerList<RemoveChildEventListener,
                                    CoordinationUnitType>
      remove_child_event_listeners_;
  CoordinationUnitEventListenerList<RemoveParentEventListener,
                                    CoordinationUnitType>
      remove_parent_event_listeners_;
  CoordinationUnitEventListenerList<PropertyChangedEventListener,
                                    mojom::PropertyType>
      storage_property_changed_event_listeners_;

  // There is nothing to filter WillBeDestroyedEventCallbacks on so the
  // CoordinationUnitType is used as a filter placeholder
  CoordinationUnitEventListenerList<WillBeDestroyedEventListener,
                                    CoordinationUnitType>
      will_be_destroyed_event_listeners_;

  DISALLOW_COPY_AND_ASSIGN(CoordinationUnitImpl);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_COORDINATION_UNIT_COORDINATION_UNIT_IMPL_H_
