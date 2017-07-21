// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device_manager.h"

#include "components/exo/data_device.h"

namespace exo {

DataDeviceManager::DataDeviceManager() {}

DataDeviceManager::~DataDeviceManager() {}

void DataDeviceManager::AddDataDevice(DataDevice* data_device) {
  data_devices_.insert(data_device);
}

void DataDeviceManager::RemoveDataDevice(DataDevice* data_device) {
  data_devices_.erase(data_device);
}

}  // namespace exo
