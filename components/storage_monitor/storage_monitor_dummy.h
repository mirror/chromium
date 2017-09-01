// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// StorageMonitorDummy is a StorageMonitor implementation that does nothing.
// It is only built on Linux without UDev support.

#ifndef COMPONENTS_STORAGE_MONITOR_STORAGE_MONITOR_DUMMY_H_
#define COMPONENTS_STORAGE_MONITOR_STORAGE_MONITOR_DUMMY_H_

#if defined(USE_UDEV)
#error "Use the regular Linux implementation instead."
#endif

#include "components/storage_monitor/storage_monitor.h"

namespace storage_monitor {

class StorageMonitorDummy : public StorageMonitor {
 public:
  StorageMonitorDummy();
  ~StorageMonitorDummy() override;

 private:
  // StorageMonitor implementation.
  void Init() override;
  bool GetStorageInfoForPath(const base::FilePath& path,
                             StorageInfo* device_info) const override;

  DISALLOW_COPY_AND_ASSIGN(StorageMonitorDummy);
};

}  // namespace storage_monitor

#endif  // COMPONENTS_STORAGE_MONITOR_STORAGE_MONITOR_DUMMY_H_
