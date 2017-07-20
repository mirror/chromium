// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_MANAGER_H_
#define COMPONENTS_EXO_DATA_DEVICE_MANAGER_H_

#include <cstdint>

#include "base/containers/flat_set.h"
#include "base/macros.h"

namespace ui {
class DropTargetEvent;
}

namespace exo {

class DataDevice;
class DataDeviceManagerDelegate;
class Surface;

enum class DndAction { kNone, kCopy, kMove, kAsk };

// Singleton object managing data transfer objects like DataDevice.
class DataDeviceManager {
 public:
  explicit DataDeviceManager(DataDeviceManagerDelegate* delegate);
  ~DataDeviceManager();
  static DataDeviceManager* GetInstance();

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
  static DataDeviceManager* instance_;
  DataDeviceManagerDelegate* const delegate_;

  DISALLOW_COPY_AND_ASSIGN(DataDeviceManager);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_H_
