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

// static
uint64_t SensorReadingSharedBuffer::GetOffset(mojom::SensorType type) {
  return (static_cast<uint64_t>(mojom::SensorType::LAST) -
          static_cast<uint64_t>(type)) *
         sizeof(SensorReadingSharedBuffer);
}

bool SensorReadingSharedBuffer::GetReading(SensorReading* result) const {
  int read_attempts = 0;
  while (!TryReadFromBuffer(result)) {
    // Only try to read this many times before failing to avoid waiting here
    // very long in case of contention with the writer.
    if (++read_attempts == kMaxReadAttemptsCount) {
      // Failed to successfully read, presumably because the hardware
      // thread was taking unusually long. Data in |result| was not updated
      // and was simply left what was there before.
      return false;
    }
  }

  return true;
}

bool SensorReadingSharedBuffer::TryReadFromBuffer(
    device::SensorReading* result) const {
  const device::OneWriterSeqLock& seq_lock = seqlock.value();
  auto version = seq_lock.ReadBegin();
  auto reading_data = reading;
  if (seq_lock.ReadRetry(version))
    return false;
  *result = reading;
  return true;
}

}  // namespace device
