// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/storage_monitor/storage_monitor_dummy.h"

namespace storage_monitor {

StorageMonitorDummy::StorageMonitorDummy() {}

StorageMonitorDummy::~StorageMonitorDummy() {}

void StorageMonitorDummy::Init() {}

bool StorageMonitorDummy::GetStorageInfoForPath(
    const base::FilePath& path,
    StorageInfo* device_info) const {
  return false;
}

StorageMonitor* StorageMonitor::CreateInternal() {
  return new StorageMonitorDummy();
}

}  // namespace storage_monitor
