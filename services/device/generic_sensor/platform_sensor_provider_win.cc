// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_provider_win.h"

#include <Sensors.h>
#include <objbase.h>

#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/task_runner_util.h"
#include "base/threading/thread.h"
#include "services/device/generic_sensor/absolute_orientation_euler_angles_fusion_algorithm_using_accelerometer_and_magnetometer.h"
#include "services/device/generic_sensor/orientation_euler_angles_fusion_algorithm_using_quaternion.h"
#include "services/device/generic_sensor/orientation_quaternion_fusion_algorithm_using_euler_angles.h"
#include "services/device/generic_sensor/platform_sensor_fusion.h"
#include "services/device/generic_sensor/platform_sensor_win.h"
#include "services/device/generic_sensor/relative_orientation_euler_angles_fusion_algorithm_using_accelerometer.h"

namespace device {

// static
PlatformSensorProviderWin* PlatformSensorProviderWin::GetInstance() {
  return base::Singleton<
      PlatformSensorProviderWin,
      base::LeakySingletonTraits<PlatformSensorProviderWin>>::get();
}

#if defined(__clang__)
// Disable optimization to work around a Clang miscompile (crbug.com/749826).
// TODO(hans): Remove once we roll past the Clang fix in r309343.
[[clang::optnone]]
#endif
void PlatformSensorProviderWin::SetSensorManagerForTesting(
    base::win::ScopedComPtr<ISensorManager> sensor_manager) {
  sensor_manager_ = sensor_manager;
}

PlatformSensorProviderWin::PlatformSensorProviderWin() = default;
PlatformSensorProviderWin::~PlatformSensorProviderWin() = default;

void PlatformSensorProviderWin::CreateSensorInternal(
    mojom::SensorType type,
    mojo::ScopedSharedBufferMapping mapping,
    const CreateSensorCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!StartSensorThread()) {
    callback.Run(nullptr);
    return;
  }

  switch (type) {
    // For ABSOLUTE_ORIENTATION_EULER_ANGLES we use a 4-way fallback approach
    // where up to 4 different sets of sensors are attempted if necessary. The
    // sensors to be used are determined in the following order:
    //   A: Use SENSOR_TYPE_INCLINOMETER_3D directly
    //   B: ABSOLUTE_ORIENTATION_QUATERNION
    //   C: TODO(juncai): Combination of ACCELEROMETER, MAGNETOMETER and
    //      GYROSCOPE
    //   D: Combination of ACCELEROMETER and MAGNETOMETER
    case mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES: {
      if (HasSensor(mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES)) {
        break;
      } else if (HasSensor(
                     mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION)) {
        std::vector<mojom::SensorType> source_sensor_types = {
            mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION};
        auto sensor_fusion_algorithm = base::MakeUnique<
            OrientationEulerAnglesFusionAlgorithmUsingQuaternion>();
        // If a PlatformSensorFusion object is created successfully, the caller
        // of this function holds the reference to the object.
        base::MakeRefCounted<PlatformSensorFusion>(
            std::move(mapping), this, callback, source_sensor_types,
            mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES,
            std::move(sensor_fusion_algorithm));
      } else if (HasSensor(mojom::SensorType::ACCELEROMETER) &&
                 HasSensor(mojom::SensorType::MAGNETOMETER)) {
        std::vector<mojom::SensorType> source_sensor_types = {
            mojom::SensorType::ACCELEROMETER, mojom::SensorType::MAGNETOMETER};
        auto sensor_fusion_algorithm = base::MakeUnique<
            AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer>();
        // If a PlatformSensorFusion object is created successfully, the caller
        // of this function holds the reference to the object.
        base::MakeRefCounted<PlatformSensorFusion>(
            std::move(mapping), this, callback, source_sensor_types,
            mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES,
            std::move(sensor_fusion_algorithm));
      } else {
        callback.Run(nullptr);
      }
      return;
    }
    // For ABSOLUTE_ORIENTATION_QUATERNION we use a 2-way fallback approach
    // where up to 2 different sets of sensors are attempted if necessary. The
    // sensors to be used are determined in the following order:
    //   A: Use SENSOR_TYPE_AGGREGATED_DEVICE_ORIENTATION directly
    //   B: ABSOLUTE_ORIENTATION_EULER_ANGLES
    case mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION: {
      if (HasSensor(mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION)) {
        break;
      } else if (HasSensor(
                     mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES) ||
                 (HasSensor(mojom::SensorType::ACCELEROMETER) &&
                  HasSensor(mojom::SensorType::MAGNETOMETER))) {
        std::vector<mojom::SensorType> source_sensor_types = {
            mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES};
        auto sensor_fusion_algorithm = base::MakeUnique<
            OrientationQuaternionFusionAlgorithmUsingEulerAngles>();
        // If a PlatformSensorFusion object is created successfully, the caller
        // of this function holds the reference to the object.
        base::MakeRefCounted<PlatformSensorFusion>(
            std::move(mapping), this, callback, source_sensor_types,
            mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION,
            std::move(sensor_fusion_algorithm));
      } else {
        callback.Run(nullptr);
      }
      return;
    }
    // For RELATIVE_ORIENTATION_EULER_ANGLES we use a 2-way fallback approach
    // where up to 2 different sets of sensors are attempted if necessary. The
    // sensors to be used are determined in the following order:
    //   A: TODO(juncai): Combination of ACCELEROMETER and GYROSCOPE
    //   B: ACCELEROMETER
    case mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES: {
      if (HasSensor(mojom::SensorType::ACCELEROMETER)) {
        std::vector<mojom::SensorType> source_sensor_types = {
            mojom::SensorType::ACCELEROMETER};
        auto sensor_fusion_algorithm = base::MakeUnique<
            RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometer>();
        // If a PlatformSensorFusion object is created successfully, the caller
        // of this function holds the reference to the object.
        base::MakeRefCounted<PlatformSensorFusion>(
            std::move(mapping), this, callback, source_sensor_types,
            mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES,
            std::move(sensor_fusion_algorithm));
      } else {
        callback.Run(nullptr);
      }
      return;
    }
    // For RELATIVE_ORIENTATION_QUATERNION we use
    // RELATIVE_ORIENTATION_EULER_ANGLES to implement it.
    case mojom::SensorType::RELATIVE_ORIENTATION_QUATERNION: {
      std::vector<mojom::SensorType> source_sensor_types = {
          mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES};
      auto sensor_fusion_algorithm = base::MakeUnique<
          OrientationQuaternionFusionAlgorithmUsingEulerAngles>();
      // If a PlatformSensorFusion object is created successfully, the caller
      // of this function holds the reference to the object.
      base::MakeRefCounted<PlatformSensorFusion>(
          std::move(mapping), this, callback, source_sensor_types,
          mojom::SensorType::RELATIVE_ORIENTATION_QUATERNION,
          std::move(sensor_fusion_algorithm));
      return;
    }
  }

  base::PostTaskAndReplyWithResult(
      sensor_thread_->task_runner().get(), FROM_HERE,
      base::Bind(&PlatformSensorProviderWin::CreateSensorReader,
                 base::Unretained(this), type),
      base::Bind(&PlatformSensorProviderWin::SensorReaderCreated,
                 base::Unretained(this), type, base::Passed(&mapping),
                 callback));
}

bool PlatformSensorProviderWin::InitializeSensorManager() {
  if (sensor_manager_)
    return true;

  HRESULT hr = ::CoCreateInstance(CLSID_SensorManager, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&sensor_manager_));
  return SUCCEEDED(hr) && sensor_manager_.Get();
}

void PlatformSensorProviderWin::AllSensorsRemoved() {
  StopSensorThread();
}

bool PlatformSensorProviderWin::StartSensorThread() {
  if (!sensor_thread_) {
    sensor_thread_ = base::MakeUnique<base::Thread>("Sensor thread");
    sensor_thread_->init_com_with_mta(true);
  }

  if (!sensor_thread_->IsRunning())
    return sensor_thread_->Start();
  return true;
}

void PlatformSensorProviderWin::StopSensorThread() {
  if (sensor_thread_ && sensor_thread_->IsRunning()) {
    sensor_manager_.Reset();
    sensor_thread_->Stop();
  }
}

void PlatformSensorProviderWin::SensorReaderCreated(
    mojom::SensorType type,
    mojo::ScopedSharedBufferMapping mapping,
    const CreateSensorCallback& callback,
    std::unique_ptr<PlatformSensorReaderWin> sensor_reader) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!sensor_reader) {
    callback.Run(nullptr);
    if (!HasSensors())
      StopSensorThread();
    return;
  }

  scoped_refptr<PlatformSensor> sensor = new PlatformSensorWin(
      type, std::move(mapping), this, sensor_thread_->task_runner(),
      std::move(sensor_reader));
  callback.Run(sensor);
}

std::unique_ptr<PlatformSensorReaderWin>
PlatformSensorProviderWin::CreateSensorReader(mojom::SensorType type) {
  DCHECK(sensor_thread_->task_runner()->BelongsToCurrentThread());
  if (!InitializeSensorManager())
    return nullptr;
  return PlatformSensorReaderWin::Create(type, sensor_manager_);
}

bool PlatformSensorProviderWin::HasSensor(mojom::SensorType sensor_type) {
  SENSOR_TYPE_ID sensor_type_id;
  switch (sensor_type) {
    case mojom::SensorType::AMBIENT_LIGHT:
      sensor_type_id = SENSOR_TYPE_AMBIENT_LIGHT;
      break;
    case mojom::SensorType::ACCELEROMETER:
      sensor_type_id = SENSOR_TYPE_ACCELEROMETER_3D;
      break;
    case mojom::SensorType::GYROSCOPE:
      sensor_type_id = SENSOR_TYPE_GYROMETER_3D;
      break;
    case mojom::SensorType::MAGNETOMETER:
      sensor_type_id = SENSOR_TYPE_COMPASS_3D;
      break;
    case mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES:
      sensor_type_id = SENSOR_TYPE_INCLINOMETER_3D;
      break;
    case mojom::SensorType::ABSOLUTE_ORIENTATION_QUATERNION:
      sensor_type_id = SENSOR_TYPE_AGGREGATED_DEVICE_ORIENTATION;
      break;
    default:
      return false;
  }

  base::win::ScopedComPtr<ISensor> sensor;
  base::win::ScopedComPtr<ISensorCollection> sensor_collection;
  HRESULT hr = sensor_manager_->GetSensorsByType(
      sensor_type_id, sensor_collection.GetAddressOf());

  if (FAILED(hr) || !sensor_collection)
    return false;

  ULONG count = 0;
  hr = sensor_collection->GetCount(&count);
  return SUCCEEDED(hr) && count > 0;
}

}  // namespace device
