// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/generic_sensor/sensor_reading_shared_buffer_reader.h"

namespace {

constexpr int kMaxReadAttemptsCount = 10;

}  // namespace

namespace device {

SensorReadingSharedBufferReader::SensorReadingSharedBufferReader(
    const SensorReadingSharedBuffer* buffer)
    : buffer_(buffer), buffer_seq_lock_(buffer_->seqlock.value()) {}

SensorReadingSharedBufferReader::~SensorReadingSharedBufferReader() = default;

bool SensorReadingSharedBufferReader::GetReading(SensorReading* result) {
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

bool SensorReadingSharedBufferReader::TryReadFromBuffer(SensorReading* result) {
  auto version = buffer_seq_lock_.ReadBegin();
  temp_reading_data_ = buffer_->reading;
  if (buffer_seq_lock_.ReadRetry(version))
    return false;
  *result = temp_reading_data_;
  return true;
}

}  // namespace device
