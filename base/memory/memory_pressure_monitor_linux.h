// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_MEMORY_PRESSURE_MONITOR_LINUX_H_
#define BASE_MEMORY_MEMORY_PRESSURE_MONITOR_LINUX_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"

namespace base {

class BASE_EXPORT MemoryPressureMonitorLinux
    : public base::MemoryPressureMonitor {
 public:
  MemoryPressureMonitorLinux();
  ~MemoryPressureMonitorLinux() override;

  void CheckMemoryPressureSoon();

  MemoryPressureLevel GetCurrentPressureLevel() override;
  void SetDispatchCallback(const DispatchCallback& callback) override;

 private:
  MemoryPressureLevel current_memory_pressure_level_;

  base::WeakPtrFactory<MemoryPressureMonitorLinux> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MemoryPressureMonitorLinux);
};

}  // namespace base

#endif  // BASE_MEMORY_MEMORY_PRESSURE_MONITOR_LINUX_H_
