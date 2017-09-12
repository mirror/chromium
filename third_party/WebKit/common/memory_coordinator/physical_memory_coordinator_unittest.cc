// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/memory_coordinator/physical_memory_coordinator.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

class PhysicalMemoryCoordinatorTest : public ::testing::Test {};

TEST_F(PhysicalMemoryCoordinatorTest, GetApproximatedDeviceMemory) {
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(128);  // 128MB
  EXPECT_EQ(0.125, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(256);  // 256MB
  EXPECT_EQ(0.25, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(510);  // <512MB
  EXPECT_EQ(0.5, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(512);  // 512MB
  EXPECT_EQ(0.5, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(640);  // 512+128MB
  EXPECT_EQ(0.5, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(768);  // 512+256MB
  EXPECT_EQ(0.75, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(1000);  // <1GB
  EXPECT_EQ(1, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(1024);  // 1GB
  EXPECT_EQ(1, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(1536);  // 1.5GB
  EXPECT_EQ(1.5, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(2000);  // <2GB
  EXPECT_EQ(2, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(2048);  // 2GB
  EXPECT_EQ(2, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(3000);  // <3GB
  EXPECT_EQ(3, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(5120);  // 5GB
  EXPECT_EQ(4, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(8192);  // 8GB
  EXPECT_EQ(8, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(16384);  // 16GB
  EXPECT_EQ(16, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(32768);  // 32GB
  EXPECT_EQ(32, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
  PhysicalMemoryCoordinator::SetPhysicalMemoryMBForTesting(64385);  // <64GB
  EXPECT_EQ(64, PhysicalMemoryCoordinator::GetApproximatedDeviceMemory());
}

}  // namespace

}  // namespace blink
