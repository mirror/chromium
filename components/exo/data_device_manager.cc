// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device_manager.h"

#include "ash/shell.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/data_device.h"
#include "ui/aura/client/drag_drop_client.h"

namespace exo {

DataDeviceManager::DataDeviceManager() = default;

void DataDeviceManager::AddDataDevice(DataDevice* device) {
  data_device_list_.AddObserver(device);
}

void DataDeviceManager::RemoveDataDevice(DataDevice* device) {
  data_device_list_.RemoveObserver(device);
}

}  // namespace exo
