// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RemapSensorReading_h
#define RemapSensorReading_h

#include "services/device/public/cpp/generic_sensor/sensor_reading.h"
#include "services/device/public/interfaces/sensor.mojom-blink.h"

namespace blink {

// For spatial sensors, remaps coordinates to the
// screen coordinate system, i.e. performs clockwise
// rotation |angle| degrees around Z-axis:
// |  cos(a) sin(a)  0||x|
// | -sin(a) cos(a)  0||y|
// |      0      0   1||z|
void RemapSensorReading(device::mojom::blink::SensorType,
                        uint16_t angle,
                        device::SensorReading*);

}  // namespace blink

#endif  // RemapSensorReading_h
