// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_SENSOR_READING_H_
#define SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_SENSOR_READING_H_

#include "device/base/synchronization/one_writer_seqlock.h"
#include "services/device/public/interfaces/sensor.mojom.h"

namespace device {

// This class is guarantied to have a fixed size of 64 bits on every platform.
// It is introduce to simplify sensors shared buffer memory calculation.
template <typename Data>
class SensorReadingField {
 public:
  static_assert(sizeof(Data) <= sizeof(int64_t),
                "The field size must be <= 64 bits.");
  SensorReadingField() = default;
  SensorReadingField(Data value) { storage_.value = value; }
  SensorReadingField& operator=(Data value) {
    storage_.value = value;
    return *this;
  }
  Data& value() { return storage_.value; }
  const Data& value() const { return storage_.value; }

  operator Data() const { return storage_.value; }

 private:
  union Storage {
    int64_t unused;
    Data value;
    Storage() { new (&value) Data(); }
    ~Storage() { value.~Data(); }
  };
  Storage storage_;
};

struct SensorReadingBase {
  SensorReadingBase();
  ~SensorReadingBase();
  SensorReadingField<double> timestamp;
};

// Represents raw sensor reading data: timestamp and 4 values.
struct SensorReadingRaw : public SensorReadingBase {
  SensorReadingRaw();
  ~SensorReadingRaw();

  constexpr static size_t kValuesCount = 4;
  SensorReadingField<double> values[kValuesCount];
};

// Represents a single data value.
struct SensorReadingSingle : public SensorReadingBase {
  SensorReadingSingle();
  ~SensorReadingSingle();
  SensorReadingField<double> value;
};

// Represents a vector in 3d coordinate system.
struct SensorReadingXYZ : public SensorReadingBase {
  SensorReadingXYZ();
  ~SensorReadingXYZ();
  SensorReadingField<double> x;
  SensorReadingField<double> y;
  SensorReadingField<double> z;
};

// Represents quaternion.
struct SensorReadingQuat : public SensorReadingXYZ {
  SensorReadingQuat();
  ~SensorReadingQuat();
  SensorReadingField<double> w;
};

union SensorReading {
  SensorReadingRaw raw;
  SensorReadingSingle als;             // AMBIENT_LIGHT
  SensorReadingXYZ accel;              // ACCELEROMETER, LINEAR_ACCELERATION
  SensorReadingXYZ gyro;               // GYROSCOPE
  SensorReadingXYZ magn;               // MAGNETOMETER
  SensorReadingQuat orientation_quat;  // ABSOLUTE_ORIENTATION_QUATERNION,
                                       // RELATIVE_ORIENTATION_QUATERNION
  SensorReadingXYZ orientation_euler;  // ABSOLUTE_ORIENTATION_EULER_ANGLES,
                                       // RELATIVE_ORIENTATION_EULER_ANGLES

  double timestamp() const { return raw.timestamp; }

  SensorReading();
  ~SensorReading();
};

static_assert(sizeof(SensorReading) == sizeof(SensorReadingRaw),
              "Check SensorReading size.");

// This structure represents sensor reading buffer: sensor reading and seqlock
// for synchronization.
struct SensorReadingSharedBuffer {
  SensorReadingSharedBuffer();
  ~SensorReadingSharedBuffer();
  SensorReadingField<OneWriterSeqLock> seqlock;

  SensorReading reading;

  // Gets the shared reading buffer offset for the given sensor type.
  static uint64_t GetOffset(mojom::SensorType type);
};

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_GENERIC_SENSOR_SENSOR_READING_H_
