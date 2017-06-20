// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/generic_sensor/public/cpp/sensor_reading.h"

#include "device/base/synchronization/shared_memory_seqlock_buffer.h"

namespace {

constexpr int kMaxReadAttemptsCount = 10;

}  // namespace

namespace device {

SensorReading::SensorReading() = default;
SensorReading::SensorReading(const SensorReading& other) = default;
SensorReading::~SensorReading() = default;

SensorReadingSharedBuffer::SensorReadingSharedBuffer() = default;
SensorReadingSharedBuffer::~SensorReadingSharedBuffer() = default;

bool SensorReadingSharedBuffer::TryReadFromBuffer(
    device::SensorReading* result) const {
  const device::OneWriterSeqLock& seq_lock = seqlock.value();
  auto version = seq_lock.ReadBegin();
  if (seq_lock.ReadRetry(version))
    return false;
  *result = reading;
  return true;
}

bool SensorReadingSharedBuffer::GetReading(SensorReading* result) const {
  int read_attempts = 0;
  while (!TryReadFromBuffer(result)) {
    if (++read_attempts == kMaxReadAttemptsCount)
      return false;
  }

  return true;
}

// static
uint64_t SensorReadingSharedBuffer::GetOffset(mojom::SensorType type) {
  return (static_cast<uint64_t>(mojom::SensorType::LAST) -
          static_cast<uint64_t>(type)) *
         sizeof(SensorReadingSharedBuffer);
}

}  // namespace device
