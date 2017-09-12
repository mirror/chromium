// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/memory_coordinator/memory_coordinator.h"

#include "base/logging.h"
#include "base/sys_info.h"

namespace blink {

// static
float WebCommonMemoryCoordinator::approximated_device_memory_gb_ = 0.0;
int64_t WebCommonMemoryCoordinator::physical_memory_mb_ = 0;

// static
int64_t WebCommonMemoryCoordinator::GetPhysicalMemoryMB() {
  return physical_memory_mb_;
}

// static
void WebCommonMemoryCoordinator::SetPhysicalMemoryMBForTesting(
    int64_t physical_memory_mb) {
  physical_memory_mb_ = physical_memory_mb;
  CalculateAndSetApproximatedDeviceMemory();
}

// static
double WebCommonMemoryCoordinator::GetApproximatedDeviceMemory() {
  return static_cast<double>(approximated_device_memory_gb_);
}

// static
void WebCommonMemoryCoordinator::CalculateAndSetApproximatedDeviceMemory() {
  // The calculations in this method are described in the specifcations:
  // https://github.com/WICG/device-memory.
  DCHECK_GT(physical_memory_mb_, 0);
  int lower_bound = physical_memory_mb_;
  int power = 0;

  // Extract the most significant 2-bits and their location.
  while (lower_bound >= 4) {
    lower_bound >>= 1;
    power++;
  }
  // The lower_bound value is either 0b10 or 0b11.
  DCHECK(lower_bound & 2);

  int64_t upper_bound = lower_bound + 1;
  lower_bound = lower_bound << power;
  upper_bound = upper_bound << power;

  // Find the closest bound, and convert it to GB.
  if (physical_memory_mb_ - lower_bound <= upper_bound - physical_memory_mb_)
    approximated_device_memory_gb_ = static_cast<float>(lower_bound) / 1024.0;
  else
    approximated_device_memory_gb_ = static_cast<float>(upper_bound) / 1024.0;
}

// static
void WebCommonMemoryCoordinator::Initialize() {
  physical_memory_mb_ = ::base::SysInfo::AmountOfPhysicalMemoryMB();
  CalculateAndSetApproximatedDeviceMemory();
}

/*
void WebMemoryCoordinator::SetPhysicalMemoryMBForTesting(
    int64_t physical_memory_mb) {
  MemoryCoordinator::SetPhysicalMemoryMBForTesting(physical_memory_mb);
}
*/

}  // namespace blink
