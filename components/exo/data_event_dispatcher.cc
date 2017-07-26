// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_event_dispatcher.h"

#include "base/logging.h"
#include "components/exo/data_device.h"
#include "ui/base/dragdrop/drag_drop_types.h"

namespace exo {

DataEventDispatcher::DataEventDispatcher() {}

DataEventDispatcher::~DataEventDispatcher() {}

void DataEventDispatcher::AddDataDevice(DataDevice* data_device) {
  DCHECK_EQ(data_devices_.count(data_device), 0u);
  data_devices_.insert(data_device);
}

void DataEventDispatcher::RemoveDataDevice(DataDevice* data_device) {
  DCHECK_EQ(data_devices_.count(data_device), 1u);
  data_devices_.erase(data_device);
}

void DataEventDispatcher::OnDragEntered(Surface* surface,
                                        const ui::DropTargetEvent& event) {
  for (DataDevice* device : data_devices_) {
    if (!device->CanAcceptDataEventsForSurface(surface))
      continue;
    device->OnDragEntered(surface, event);
  }
}

int DataEventDispatcher::OnDragUpdated(Surface* surface,
                                       const ui::DropTargetEvent& event) {
  int valid_operation = ui::DragDropTypes::DRAG_NONE;
  for (DataDevice* device : data_devices_) {
    if (!device->CanAcceptDataEventsForSurface(surface))
      continue;
    valid_operation = valid_operation | device->OnDragUpdated(event);
  }
  return valid_operation;
}

void DataEventDispatcher::OnDragExited(Surface* surface) {
  for (DataDevice* device : data_devices_) {
    if (!device->CanAcceptDataEventsForSurface(surface))
      continue;
    device->OnDragExited();
  }
}

int DataEventDispatcher::OnPerformDrop(Surface* surface,
                                       const ui::DropTargetEvent& event) {
  for (DataDevice* device : data_devices_) {
    if (!device->CanAcceptDataEventsForSurface(surface))
      continue;
    device->OnPerformDrop(event);
  }
  // TODO(hirono): Return the correct result instead of always returning
  // DRAG_MOVE.
  return ui::DragDropTypes::DRAG_MOVE;
}

}  // namespace exo
