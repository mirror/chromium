// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/memory_pressure_monitor_linux.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

namespace base {

MemoryPressureMonitorLinux::MemoryPressureMonitorLinux()
    : current_memory_pressure_level_(
          MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE),
      weak_ptr_factory_(this) {}

MemoryPressureMonitorLinux::~MemoryPressureMonitorLinux() {}

MemoryPressureListener::MemoryPressureLevel
MemoryPressureMonitorLinux::GetCurrentPressureLevel() {
  return current_memory_pressure_level_;
}

void MemoryPressureMonitorLinux::SetDispatchCallback(
    const DispatchCallback& callback) {}

}  // namespace base
