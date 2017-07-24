// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_event_dispatcher.h"

#include "base/logging.h"
#include "components/exo/data_device.h"

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

}  // namespace exo
