// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/platform_sensor_provider.h"

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "services/device/device_service_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class FakePlatformSensor : public PlatformSensor {
 public:
  FakePlatformSensor(mojom::SensorType type,
                     mojo::ScopedSharedBufferMapping mapping,
                     PlatformSensorProvider* provider)
      : PlatformSensor(type, std::move(mapping), provider) {}

  bool StartSensor(const PlatformSensorConfiguration& configuration) override {
    return true;
  }

  void StopSensor() override {}

  bool CheckSensorConfiguration(
      const PlatformSensorConfiguration& configuration) override {
    return true;
  }

  PlatformSensorConfiguration GetDefaultConfiguration() override {
    return PlatformSensorConfiguration(30.0);
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
  static FakePlatformSensorProvider* GetInstance() {
    return base::Singleton<
        FakePlatformSensorProvider,
        base::LeakySingletonTraits<FakePlatformSensorProvider>>::get();
  }

  FakePlatformSensorProvider() = default;
  ~FakePlatformSensorProvider() override = default;

  enum RequestResult { kSuccess, kFailure, kPending };

  void set_next_request_result(RequestResult result) {
    request_result_ = result;
  }

  MOCK_METHOD0(FreeResources, void());

 private:
  void CreateSensorInternal(mojom::SensorType type,
                            mojo::ScopedSharedBufferMapping mapping,
                            const CreateSensorCallback& callback) override {
    if (request_result_ == kFailure) {
      request_result_ = kSuccess;
      callback.Run(nullptr);
      return;
    }

    if (request_result_ == kPending) {
      request_result_ = kSuccess;
      return;
    }

    auto sensor = base::MakeRefCounted<FakePlatformSensor>(
        type, std::move(mapping), this);
    callback.Run(std::move(sensor));
  }

  RequestResult request_result_ = kSuccess;
  DISALLOW_COPY_AND_ASSIGN(FakePlatformSensorProvider);
};

class PlatformSensorProviderTest : public DeviceServiceTestBase {
 public:
  PlatformSensorProviderTest() = default;

  void SetUp() override { provider_.reset(new FakePlatformSensorProvider); }

  void TearDown() override { provider_.reset(); }

 protected:
  std::unique_ptr<FakePlatformSensorProvider> provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformSensorProviderTest);
};

TEST_F(PlatformSensorProviderTest, ResourcesAreFreed) {
  EXPECT_CALL(*provider_, FreeResources()).Times(2);

  provider_->set_next_request_result(FakePlatformSensorProvider::kFailure);
  provider_->CreateSensor(
      device::mojom::SensorType::AMBIENT_LIGHT,
      base::Bind([](scoped_refptr<PlatformSensor> s) { EXPECT_FALSE(s); }));

  provider_->CreateSensor(
      device::mojom::SensorType::AMBIENT_LIGHT,
      base::Bind([](scoped_refptr<PlatformSensor> s) { EXPECT_TRUE(s); }));
}

TEST_F(PlatformSensorProviderTest, ResourcesAreNotFreedOnPendingRequest) {
  EXPECT_CALL(*provider_, FreeResources()).Times(0);

  provider_->set_next_request_result(FakePlatformSensorProvider::kPending);
  provider_->CreateSensor(
      device::mojom::SensorType::AMBIENT_LIGHT,
      base::Bind([](scoped_refptr<PlatformSensor> s) { EXPECT_FALSE(s); }));

  provider_->CreateSensor(
      device::mojom::SensorType::AMBIENT_LIGHT,
      base::Bind([](scoped_refptr<PlatformSensor> s) { EXPECT_TRUE(s); }));
}

}  // namespace device
