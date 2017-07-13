// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device.h"

#include "ash/shell.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/data_device_manager.h"
#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/window.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"

namespace exo {

DataDeviceDelegate::~DataDeviceDelegate() = default;

DataDevice::DataDevice(DataDeviceManager* manager,
                       std::unique_ptr<DataDeviceDelegate> delegate)
    : data_device_manager_(manager), delegate_(std::move(delegate)) {
  data_device_manager_->AddDataDevice(this);
}

DataDevice::~DataDevice() {
  data_device_manager_->RemoveDataDevice(this);
}

void DataDevice::StartDrag(const DataSource* source_resource,
                           Surface* origin_resource,
                           Surface* icon_resource,
                           uint32_t serial) {
  NOTIMPLEMENTED();
}

void DataDevice::SetSelection(const DataSource& source, uint32_t serial) {
  NOTIMPLEMENTED();
}

}  // namespace exo
