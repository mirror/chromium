// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device_manager.h"

#include "base/logging.h"
#include "components/exo/data_device.h"
#include "components/exo/data_device_manager_delegate.h"
#include "ui/base/dragdrop/drag_drop_types.h"

namespace exo {

DataDeviceManager* DataDeviceManager::instance_ = nullptr;

DataDeviceManager::DataDeviceManager(DataDeviceManagerDelegate* delegate)
    : delegate_(delegate) {
  DCHECK(!instance_);
  instance_ = this;
}

DataDeviceManager::~DataDeviceManager() {
  delegate_->OnDataDeviceManagerDestroying(this);
  instance_ = nullptr;
}

DataDeviceManager* DataDeviceManager::GetInstance() {
  return instance_;
}

void DataDeviceManager::AddDataDevice(DataDevice* data_device) {
  delegate_->AddDataDevice(data_device);
}

void DataDeviceManager::RemoveDataDevice(DataDevice* data_device) {
  delegate_->RemoveDataDevice(data_device);
}

void DataDeviceManager::OnDragEntered(Surface* surface,
                                      const ui::DropTargetEvent& event) {
  for (DataDevice* device : delegate_->GetDataDevicesForSurface(surface))
    device->OnDragEntered(surface, event);
}

int DataDeviceManager::OnDragUpdated(Surface* surface,
                                     const ui::DropTargetEvent& event) {
  int valid_operation = ui::DragDropTypes::DRAG_NONE;
  for (DataDevice* device : delegate_->GetDataDevicesForSurface(surface))
    valid_operation = valid_operation | device->OnDragUpdated(event);
  return valid_operation;
}

void DataDeviceManager::OnDragExited(Surface* surface) {
  for (DataDevice* device : delegate_->GetDataDevicesForSurface(surface))
    device->OnDragExited();
}

int DataDeviceManager::OnPerformDrop(Surface* surface,
                                     const ui::DropTargetEvent& event) {
  for (DataDevice* device : delegate_->GetDataDevicesForSurface(surface))
    device->OnPerformDrop(event);
  // TODO(hirono): Return the correct result instead of always returning
  // DRAG_MOVE.
  return ui::DragDropTypes::DRAG_MOVE;
}

}  // namespace exo
