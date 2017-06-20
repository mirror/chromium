// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/generic_sensor/public/cpp/sensor_reading.h"

#include "device/base/synchronization/shared_memory_seqlock_buffer.h"

namespace {

constexpr int kMaxReadAttemptsCount = 10;

bool TryReadFromBuffer(const device::SensorReadingSharedBuffer* buffer,
                       device::SensorReading* result) {
  const device::OneWriterSeqLock& seqlock = buffer->seqlock.value();
  auto version = seqlock.ReadBegin();
  auto reading_data = buffer->reading;
  if (seqlock.ReadRetry(version))
    return false;
  *result = reading_data;
  return true;
}

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

bool GetSensorReadingFromBuffer(const SensorReadingSharedBuffer* buffer,
                                SensorReading* result) {
  int read_attempts = 0;
  while (!TryReadFromBuffer(buffer, result)) {
    if (++read_attempts == kMaxReadAttemptsCount)
      return false;
  }

  return true;
}

}  // namespace device
