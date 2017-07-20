// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_MANAGER_DELEGATE_H_
#define COMPONENTS_EXO_DATA_DEVICE_MANAGER_DELEGATE_H_

#include "base/containers/flat_set.h"

namespace exo {

class DataDevice;
class Surface;

// Handles events on the data device manager in context-specific ways.
class DataDeviceManagerDelegate {
 public:
  // Called at the top of the data device's destructor, to give observers a
  // chance to remove themselves.
  virtual void OnDataDeviceManagerDestroying(
      DataDeviceManager* data_device_manager) = 0;

  // Adds |data_device| to the manager.
  virtual void AddDataDevice(DataDevice* data_device) = 0;

  // Removes |data_device| from the manager.
  virtual void RemoveDataDevice(DataDevice* data_device) = 0;

  // Returns data devices which should be notified events on |surface|.
  virtual base::flat_set<DataDevice*> GetDataDevicesForSurface(
      Surface* surface) = 0;

 protected:
  virtual ~DataDeviceManagerDelegate() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_MANAGER_DELEGATE_H_
