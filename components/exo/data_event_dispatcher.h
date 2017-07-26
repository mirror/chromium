// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_EVEMT_DISPATCHER_H_
#define COMPONENTS_EXO_DATA_EVEMT_DISPATCHER_H_

#include "base/containers/flat_set.h"
#include "base/macros.h"

namespace ui {
class DropTargetEvent;
}

namespace exo {

class DataDevice;
class Surface;

// Object dispatching events to data_device.
class DataEventDispatcher {
 public:
  DataEventDispatcher();
  ~DataEventDispatcher();

  // Adds |data_device| to the manager.
  void AddDataDevice(DataDevice* data_device);

  // Removes |data_device| from the manager.
  void RemoveDataDevice(DataDevice* data_device);

  // Called when dragging cursor enters the |surface|.
  void OnDragEntered(Surface* surface, const ui::DropTargetEvent& event);

  // Called when dragging cursor updates on the |surface|.
  int OnDragUpdated(Surface* surface, const ui::DropTargetEvent& event);

  // Called when dragging cursor leaves the |surface|.
  void OnDragExited(Surface* surface);

  // Called when drop operation is performed on the |surface|.
  int OnPerformDrop(Surface* surface, const ui::DropTargetEvent& event);

 private:
  base::flat_set<DataDevice*> data_devices_;

  DISALLOW_COPY_AND_ASSIGN(DataEventDispatcher);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_EVEMT_DISPATCHER_H_
