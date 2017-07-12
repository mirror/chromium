// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_SENSOR_READING_SHARED_BUFFER_READER_H_
#define SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_SENSOR_READING_SHARED_BUFFER_READER_H_

#include "base/macros.h"
#include "device/base/synchronization/shared_memory_seqlock_buffer.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading.h"

namespace device {

class SensorReadingSharedBufferReader {
 public:
  explicit SensorReadingSharedBufferReader(
      const SensorReadingSharedBuffer* buffer);
  ~SensorReadingSharedBufferReader();

  // Get sensor reading from shared buffer.
  bool GetReading(SensorReading* result);

 private:
  bool TryReadFromBuffer(SensorReading* result);

  const SensorReadingSharedBuffer* buffer_;
  const device::OneWriterSeqLock& buffer_seq_lock_;
  SensorReading temp_reading_data_;

  DISALLOW_COPY_AND_ASSIGN(SensorReadingSharedBufferReader);
};

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_SENSOR_READING_SHARED_BUFFER_READER_H_
