// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/device_service_test_base.h"
#include "services/device/generic_sensor/absolute_orientation_euler_angles_fusion_algorithm_using_accelerometer_and_magnetometer.h"
#include "services/device/generic_sensor/linear_acceleration_fusion_algorithm_using_accelerometer.h"
#include "services/device/generic_sensor/platform_sensor.h"
#include "services/device/generic_sensor/platform_sensor_fusion.h"
#include "services/device/generic_sensor/platform_sensor_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

using mojom::SensorType;

class FakePlatformSensor : public PlatformSensor {
 public:
  FakePlatformSensor(SensorType type,
                     mojo::ScopedSharedBufferMapping mapping,
                     PlatformSensorProvider* provider)
      : PlatformSensor(type, std::move(mapping), provider) {}

  bool StartSensor(const PlatformSensorConfiguration& configuration) override {
    return true;
  }

  void StopSensor() override {}

  bool CheckSensorConfiguration(
      const PlatformSensorConfiguration& configuration) override {
    return configuration.frequency() <= GetMaximumSupportedFrequency() &&
           configuration.frequency() >= GetMinimumSupportedFrequency();
  }

  PlatformSensorConfiguration GetDefaultConfiguration() override {
    PlatformSensorConfiguration default_configuration;
    default_configuration.set_frequency(30.0);
    return default_configuration;
  }

  mojom::ReportingMode GetReportingMode() override {
    return mojom::ReportingMode::ON_CHANGE;
  }

 protected:
  ~FakePlatformSensor() override = default;

  DISALLOW_COPY_AND_ASSIGN(FakePlatformSensor);
};

class FakePlatformSensorProvider : public PlatformSensorProvider {
 public:
  FakePlatformSensorProvider() = default;
  ~FakePlatformSensorProvider() override = default;

  void CreateSensorInternal(SensorType type,
                            mojo::ScopedSharedBufferMapping mapping,
                            const CreateSensorCallback& callback) override {
    DCHECK(type >= SensorType::FIRST && type <= SensorType::LAST);

    switch (type) {
      case SensorType::ACCELEROMETER:
        if (!accelerometer_is_available_) {
          callback.Run(nullptr);
          return;
        }
        break;
      case SensorType::MAGNETOMETER:
        if (!magnetometer_is_available_) {
          callback.Run(nullptr);
          return;
        }
        break;
      default:
        break;
    }

    auto sensor = base::MakeRefCounted<FakePlatformSensor>(
        type, std::move(mapping), this);
    callback.Run(sensor);
  }

  void set_accelerometer_is_available(bool accelerometer_is_available) {
    accelerometer_is_available_ = accelerometer_is_available;
  }

  void set_magnetometer_is_available(bool magnetometer_is_available) {
    magnetometer_is_available_ = magnetometer_is_available;
  }

 private:
  bool accelerometer_is_available_ = true;
  bool magnetometer_is_available_ = true;

  DISALLOW_COPY_AND_ASSIGN(FakePlatformSensorProvider);
};

class PlatformSensorFusionTest : public DeviceServiceTestBase {
 public:
  PlatformSensorFusionTest() {
    fake_platform_sensor_provider_ =
        base::MakeUnique<FakePlatformSensorProvider>();
    PlatformSensorProvider::SetProviderForTesting(
        fake_platform_sensor_provider_.get());

    linear_acceleration_fusion_algorithm_ =
        base::MakeUnique<LinearAccelerationFusionAlgorithmUsingAccelerometer>();

    absolute_orientation_euler_angles_fusion_algorithm_ = base::MakeUnique<
        AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer>();

    callback_ =
        base::Bind(&PlatformSensorFusionTest::CreateFusionSensorCallback,
                   base::Unretained(this));
  }

  void CreateFusionSensorCallback(scoped_refptr<PlatformSensor> sensor) {
    create_fusion_sensor_callback_called_ = true;
    fusion_sensor_ = std::move(sensor);
  }

 protected:
  mojo::ScopedSharedBufferMapping shared_buffer_;
  std::unique_ptr<FakePlatformSensorProvider> fake_platform_sensor_provider_;
  std::unique_ptr<LinearAccelerationFusionAlgorithmUsingAccelerometer>
      linear_acceleration_fusion_algorithm_;
  std::unique_ptr<
      AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer>
      absolute_orientation_euler_angles_fusion_algorithm_;
  PlatformSensorProviderBase::CreateSensorCallback callback_;
  bool create_fusion_sensor_callback_called_ = false;
  scoped_refptr<PlatformSensor> fusion_sensor_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformSensorFusionTest);
};

// The following code tests creating a fusion sensor that needs one source
// sensor.

TEST_F(PlatformSensorFusionTest, SourceSensorAlreadyExists) {
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));
  fake_platform_sensor_provider_->CreateSensor(SensorType::ACCELEROMETER,
                                               callback_);
  // Now the source sensor already exists.
  EXPECT_TRUE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(linear_acceleration_fusion_algorithm_), callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_TRUE(fusion_sensor_);
  EXPECT_EQ(SensorType::LINEAR_ACCELERATION, fusion_sensor_->GetType());
}

TEST_F(PlatformSensorFusionTest, SourceSensorNeedsToBeCreated) {
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(linear_acceleration_fusion_algorithm_), callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_TRUE(fusion_sensor_);
  EXPECT_EQ(SensorType::LINEAR_ACCELERATION, fusion_sensor_->GetType());
}

TEST_F(PlatformSensorFusionTest, SourceSensorIsNotAvailable) {
  fake_platform_sensor_provider_->set_accelerometer_is_available(false);

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(linear_acceleration_fusion_algorithm_), callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_FALSE(fusion_sensor_);
}

// The following code tests creating a fusion sensor that needs two source
// sensors.

TEST_F(PlatformSensorFusionTest, BothSourceSensorsAlreadyExist) {
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));
  fake_platform_sensor_provider_->CreateSensor(SensorType::ACCELEROMETER,
                                               callback_);
  EXPECT_TRUE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));

  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::MAGNETOMETER));
  fake_platform_sensor_provider_->CreateSensor(SensorType::MAGNETOMETER,
                                               callback_);
  EXPECT_TRUE(
      fake_platform_sensor_provider_->GetSensor(SensorType::MAGNETOMETER));

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(absolute_orientation_euler_angles_fusion_algorithm_),
      callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_TRUE(fusion_sensor_);
  EXPECT_EQ(SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES,
            fusion_sensor_->GetType());
}

TEST_F(PlatformSensorFusionTest, BothSourceSensorsNeedToBeCreated) {
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::MAGNETOMETER));

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(absolute_orientation_euler_angles_fusion_algorithm_),
      callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_TRUE(fusion_sensor_);
  EXPECT_EQ(SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES,
            fusion_sensor_->GetType());
}

TEST_F(PlatformSensorFusionTest, BothSourceSensorsAreNotAvailable) {
  fake_platform_sensor_provider_->set_accelerometer_is_available(false);
  fake_platform_sensor_provider_->set_magnetometer_is_available(false);

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(absolute_orientation_euler_angles_fusion_algorithm_),
      callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_FALSE(fusion_sensor_);
}

TEST_F(PlatformSensorFusionTest,
       OneSourceSensorAlreadyExistsTheOtherSourceSensorNeedsToBeCreated) {
  fake_platform_sensor_provider_->CreateSensor(SensorType::ACCELEROMETER,
                                               callback_);
  EXPECT_TRUE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::MAGNETOMETER));

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(absolute_orientation_euler_angles_fusion_algorithm_),
      callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_TRUE(fusion_sensor_);
  EXPECT_EQ(SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES,
            fusion_sensor_->GetType());
}

TEST_F(PlatformSensorFusionTest,
       OneSourceSensorAlreadyExistsTheOtherSourceSensorIsNotAvailable) {
  fake_platform_sensor_provider_->CreateSensor(SensorType::ACCELEROMETER,
                                               callback_);
  EXPECT_TRUE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));

  fake_platform_sensor_provider_->set_magnetometer_is_available(false);

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(absolute_orientation_euler_angles_fusion_algorithm_),
      callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_FALSE(fusion_sensor_);
}

TEST_F(PlatformSensorFusionTest,
       OneSourceSensorNeedsToBeCreatedTheOtherSourceSensorIsNotAvailable) {
  EXPECT_FALSE(
      fake_platform_sensor_provider_->GetSensor(SensorType::ACCELEROMETER));
  fake_platform_sensor_provider_->set_magnetometer_is_available(false);

  PlatformSensorFusion::Create(
      std::move(shared_buffer_), fake_platform_sensor_provider_.get(),
      std::move(absolute_orientation_euler_angles_fusion_algorithm_),
      callback_);
  EXPECT_TRUE(create_fusion_sensor_callback_called_);
  EXPECT_FALSE(fusion_sensor_);
}

}  //  namespace device
