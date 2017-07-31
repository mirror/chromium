// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_FUSION_ALGORITHM_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_FUSION_ALGORITHM_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading.h"

namespace device {

class PlatformSensor;

// Base class for platform sensor fusion algorithm.
class PlatformSensorFusionAlgorithm {
 public:
  PlatformSensorFusionAlgorithm();
  virtual ~PlatformSensorFusionAlgorithm();

  void set_threshold(double threshold) { threshold_ = threshold; }

  void set_source_sensors(
      const std::vector<scoped_refptr<PlatformSensor>>& source_sensors);

  bool IsReadingSignificantlyDifferent(const SensorReading& reading1,
                                       const SensorReading& reading2);

  virtual bool GetFusedData(mojom::SensorType which_sensor_changed,
                            SensorReading* fused_reading) = 0;

 protected:
  bool UpdateReadings();

  std::vector<SensorReading> readings_;

 private:
  // Default threshold for comparing SensorReading values. If a
  // different threshold is better for a certain sensor type, set_threshold()
  // should be used to change it.
  double threshold_ = 0.1;

  std::vector<scoped_refptr<PlatformSensor>> source_sensors_;

  DISALLOW_COPY_AND_ASSIGN(PlatformSensorFusionAlgorithm);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_FUSION_ALGORITHM_H_
