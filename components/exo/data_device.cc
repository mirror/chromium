// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_device.h"

#include "base/logging.h"
#include "components/exo/data_device_delegate.h"
#include "components/exo/data_event_dispatcher.h"

namespace exo {

DataDevice::DataDevice(DataDeviceDelegate* delegate,
                       DataEventDispatcher* data_event_dispatcher)
    : data_event_dispatcher_(data_event_dispatcher), delegate_(delegate) {
  data_event_dispatcher_->AddDataDevice(this);
}

DataDevice::~DataDevice() {
  delegate_->OnDataDeviceDestroying(this);
  data_event_dispatcher_->RemoveDataDevice(this);
}

void DataDevice::StartDrag(const DataSource* source_resource,
                           Surface* origin_resource,
                           Surface* icon_resource,
                           uint32_t serial) {
  // TODO(hirono): Check if serial is valid. crbug.com/746111
  NOTIMPLEMENTED();
}

void DataDevice::SetSelection(const DataSource* source, uint32_t serial) {
  // TODO(hirono): Check if serial is valid. crbug.com/746111
  NOTIMPLEMENTED();
}

bool DataDevice::CanAcceptDataEventsForSurface(Surface* surface) const {
  return delegate_->CanAcceptDataEventsForSurface(surface);
}

}  // namespace exo
