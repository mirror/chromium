// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device_manager.h"

#include "base/logging.h"
#include "components/exo/data_device_manager_delegate.h"

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

base::flat_set<DataDevice*> DataDeviceManager::GetDataDevicesForSurface(
    Surface* surface) {
  return delegate_->GetDataDevicesForSurface(surface);
}

}  // namespace exo
