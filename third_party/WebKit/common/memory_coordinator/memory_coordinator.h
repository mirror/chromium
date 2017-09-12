// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_MEMORY_COORDINATOR_MEMORY_COORDINATOR_H_
#define THIRD_PARTY_WEBKIT_COMMON_MEMORY_COORDINATOR_MEMORY_COORDINATOR_H_

#include <stddef.h>
#include <stdint.h>

#include "third_party/WebKit/common/common_export.h"

namespace blink {

class WebCommonMemoryCoordinator {
 public:
  // BLINK_PLATFORM_EXPORT static double GetApproximatedDeviceMemory();

  // BLINK_PLATFORM_EXPORT static void SetPhysicalMemoryMBForTesting(int64_t);
  static void BLINK_COMMON_EXPORT CalculateAndSetApproximatedDeviceMemory();

  // Returns the amount of physical memory in megabytes on the device.
  static int64_t BLINK_COMMON_EXPORT GetPhysicalMemoryMB();

  // Override the value of the physical memory for testing.
  static void BLINK_COMMON_EXPORT SetPhysicalMemoryMBForTesting(int64_t);

  // Returns an approximation of the physical memory rounded to to the most
  // significant 2-bits. This information is provided to web-developers to allow
  // them to customize the experience of their page to the possible available
  // device memory.
  static double BLINK_COMMON_EXPORT GetApproximatedDeviceMemory();

  // Caches whether this device is a low-end device and the device physical
  // memory in static members. instance() is not used as it's a heap allocated
  // object - meaning it's not thread-safe as well as might break tests counting
  // the heap size.
  static void BLINK_COMMON_EXPORT Initialize();

 private:
  static float approximated_device_memory_gb_;
  static int64_t physical_memory_mb_;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_MEMORY_COORDINATOR_MEMORY_COORDINATOR_H_
