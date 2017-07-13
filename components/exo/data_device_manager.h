// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_DATA_DEVICE_MANAGER_H_
#define COMPONENTS_EXO_DATA_DEVICE_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/env_observer.h"

namespace exo {

class DataDevice;

class DataDeviceManager {
 public:
  DataDeviceManager();
  void AddDataDevice(DataDevice* device);
  void RemoveDataDevice(DataDevice* device);

 private:
  base::ObserverList<DataDevice> data_device_list_;

  DISALLOW_COPY_AND_ASSIGN(DataDeviceManager);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_DATA_DEVICE_MANAGER_H_
