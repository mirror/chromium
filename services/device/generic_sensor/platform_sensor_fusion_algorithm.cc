// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_fusion_algorithm.h"

#include <cmath>

namespace device {

PlatformSensorFusionAlgorithm::PlatformSensorFusionAlgorithm(
    const std::vector<mojom::SensorType>& source_types)
    : source_types_(source_types) {
  DCHECK(!source_types_.empty());
}

PlatformSensorFusionAlgorithm::~PlatformSensorFusionAlgorithm() = default;

bool PlatformSensorFusionAlgorithm::IsReadingSignificantlyDifferent(
    const SensorReading& reading1,
    const SensorReading& reading2) {
  for (size_t i = 0; i < SensorReadingRaw::kValuesCount; ++i) {
    if (std::fabs(reading1.raw.values[i] - reading2.raw.values[i]) >=
        threshold_)
      return true;
  }
  return false;
}

void PlatformSensorFusionAlgorithm::Reset() {}

void PlatformSensorFusionAlgorithm::SetFrequency(double) {}

}  // namespace device
