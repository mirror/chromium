// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/frame_coordination_unit_impl.h"

#include "services/resource_coordinator/coordination_unit/page_coordination_unit_impl.h"
#include "services/resource_coordinator/coordination_unit/process_coordination_unit_impl.h"
#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"

namespace resource_coordinator {

FrameCoordinationUnitImpl::FrameCoordinationUnitImpl(
    const CoordinationUnitID& id,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : CoordinationUnitInterface(id, std::move(service_ref)),
      parent_frame_coordination_unit_(nullptr),
      page_coordination_unit_(nullptr),
      process_coordination_unit_(nullptr) {}

FrameCoordinationUnitImpl::~FrameCoordinationUnitImpl() {
  if (parent_frame_coordination_unit_)
    parent_frame_coordination_unit_->RemoveChildFrame(this);
  if (page_coordination_unit_)
    page_coordination_unit_->RemoveFrame(this);
  if (process_coordination_unit_)
    process_coordination_unit_->RemoveFrame(this);
  for (auto* child_frame : child_frame_coordination_units_)
    child_frame->RemoveParentFrame(this);
}

void FrameCoordinationUnitImpl::AddChildFrame(const CoordinationUnitID& cu_id) {
  DCHECK(cu_id != id());
  FrameCoordinationUnitImpl* frame_cu =
      FrameCoordinationUnitImpl::GetCoordinationUnitByID(cu_id);
  if (!frame_cu)
    return;
  if (HasAncestor(frame_cu) || frame_cu->HasDescendant(this)) {
    DCHECK(false) << "Cyclic reference in coordination unit graph detected!";
    return;
  }
  if (AddChildFrame(frame_cu)) {
    frame_cu->AddParentFrame(this);
  }
}

void FrameCoordinationUnitImpl::RemoveChildFrame(
    const CoordinationUnitID& cu_id) {
  DCHECK(cu_id != id());
  FrameCoordinationUnitImpl* frame_cu =
      FrameCoordinationUnitImpl::GetCoordinationUnitByID(cu_id);
  if (!frame_cu)
    return;
  if (RemoveChildFrame(frame_cu)) {
    frame_cu->RemoveParentFrame(this);
  }
}

void FrameCoordinationUnitImpl::SetAudibility(bool audible) {
  SetProperty(mojom::PropertyType::kAudible, audible);
}

void FrameCoordinationUnitImpl::SetNetworkAlmostIdleness(bool idle) {
  SetProperty(mojom::PropertyType::kNetworkAlmostIdle, idle);
}

void FrameCoordinationUnitImpl::OnAlertFired() {
  SendEvent(mojom::Event::kAlertFired);
}

void FrameCoordinationUnitImpl::OnNonPersistentNotificationCreated() {
  SendEvent(mojom::Event::kNonPersistentNotificationCreated);
}

void FrameCoordinationUnitImpl::AddPageCoordinationUnit(
    PageCoordinationUnitImpl* page_coordination_unit) {
  DCHECK(!page_coordination_unit_);
  page_coordination_unit_ = page_coordination_unit;
}

void FrameCoordinationUnitImpl::AddProcessCoordinationUnit(
    ProcessCoordinationUnitImpl* process_coordination_unit) {
  DCHECK(!process_coordination_unit_);
  process_coordination_unit_ = process_coordination_unit;
}

void FrameCoordinationUnitImpl::RemovePageCoordinationUnit(
    PageCoordinationUnitImpl* page_coordination_unit) {
  DCHECK(page_coordination_unit == page_coordination_unit_);
  page_coordination_unit_ = nullptr;
}

void FrameCoordinationUnitImpl::RemoveProcessCoordinationUnit(
    ProcessCoordinationUnitImpl* process_coordination_unit) {
  DCHECK(process_coordination_unit == process_coordination_unit_);
  process_coordination_unit_ = nullptr;
}

FrameCoordinationUnitImpl*
FrameCoordinationUnitImpl::GetParentFrameCoordinationUnit() const {
  return parent_frame_coordination_unit_;
}

PageCoordinationUnitImpl* FrameCoordinationUnitImpl::GetPageCoordinationUnit()
    const {
  return page_coordination_unit_;
}

ProcessCoordinationUnitImpl*
FrameCoordinationUnitImpl::GetProcessCoordinationUnit() const {
  return process_coordination_unit_;
}

bool FrameCoordinationUnitImpl::IsMainFrame() const {
  return !parent_frame_coordination_unit_;
}

bool FrameCoordinationUnitImpl::HasAncestor(CoordinationUnitBase* ancestor) {
  auto cu_type = ancestor->id().type;
  if (cu_type == CoordinationUnitType::kPage) {
    if (page_coordination_unit_ && page_coordination_unit_ == ancestor) {
      return true;
    }
    return false;
  } else if (cu_type == CoordinationUnitType::kProcess) {
    if (process_coordination_unit_ && process_coordination_unit_ == ancestor) {
      return true;
    }
    return false;
  } else if (cu_type == CoordinationUnitType::kFrame &&
             parent_frame_coordination_unit_) {
    if (parent_frame_coordination_unit_ == ancestor ||
        parent_frame_coordination_unit_->HasAncestor(ancestor)) {
      return true;
    }
  }
  return false;
}

bool FrameCoordinationUnitImpl::HasDescendant(
    CoordinationUnitBase* descendant) {
  DCHECK(descendant->id().type == CoordinationUnitType::kFrame);
  for (FrameCoordinationUnitImpl* child : child_frame_coordination_units_) {
    if (child == descendant || child->HasDescendant(descendant))
      return true;
  }
  return false;
}

void FrameCoordinationUnitImpl::OnEventReceived(mojom::Event event) {
  for (auto& observer : observers())
    observer.OnFrameEventReceived(this, event);
}

void FrameCoordinationUnitImpl::OnPropertyChanged(
    mojom::PropertyType property_type,
    int64_t value) {
  for (auto& observer : observers())
    observer.OnFramePropertyChanged(this, property_type, value);
}

void FrameCoordinationUnitImpl::AddParentFrame(
    FrameCoordinationUnitImpl* parent_frame_cu) {
  parent_frame_coordination_unit_ = parent_frame_cu;
  // TODO Figure out how to notify observers.
}

bool FrameCoordinationUnitImpl::AddChildFrame(
    FrameCoordinationUnitImpl* child_frame_cu) {
  bool success =
      child_frame_coordination_units_.count(child_frame_cu)
          ? false
          : child_frame_coordination_units_.insert(child_frame_cu).second;
  return success;
}

void FrameCoordinationUnitImpl::RemoveParentFrame(
    FrameCoordinationUnitImpl* parent_frame_cu) {
  DCHECK(parent_frame_coordination_unit_ == parent_frame_cu);
  parent_frame_coordination_unit_ = nullptr;
}

bool FrameCoordinationUnitImpl::RemoveChildFrame(
    FrameCoordinationUnitImpl* child_frame_cu) {
  return child_frame_coordination_units_.erase(child_frame_cu) > 0;
}

}  // namespace resource_coordinator
