// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "device/base/synchronization/one_writer_seqlock.h"
#include "device/generic_sensor/platform_sensor.h"
#include "device/generic_sensor/platform_sensor_provider.h"
#include "device/generic_sensor/public/cpp/sensor_reading.h"
#include "device/generic_sensor/sensor_provider_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/device_service_test_base.h"
#include "services/device/public/cpp/device_features.h"
#include "services/device/public/interfaces/constants.mojom.h"

namespace device {

namespace {

void CheckValue(double expect, double value) {
  EXPECT_TRUE(expect == value);
}

void CheckSuccess(base::Closure quit_closure, bool expect, bool is_success) {
  EXPECT_TRUE(expect == is_success);
  quit_closure.Run();
}

void DoNothing() {}

class FakeAmbientLightSensor : public device::PlatformSensor {
 public:
  FakeAmbientLightSensor(device::mojom::SensorType type,
                         mojo::ScopedSharedBufferMapping mapping,
                         device::PlatformSensorProvider* provider)
      : PlatformSensor(type, std::move(mapping), provider) {}

  bool StartSensor(
      const device::PlatformSensorConfiguration& configuration) override {
    device::SensorReading reading;
    // Set the shared buffer value as frequency for testing purpose.
    reading.values[0] = configuration.frequency();
    UpdateSensorReading(reading, true);
    return true;
  }

  void StopSensor() override {}

  bool CheckSensorConfiguration(
      const device::PlatformSensorConfiguration& configuration) override {
    return configuration.frequency() <= GetMaximumSupportedFrequency() &&
           configuration.frequency() >= GetMinimumSupportedFrequency();
  }

  device::PlatformSensorConfiguration GetDefaultConfiguration() override {
    device::PlatformSensorConfiguration default_configuration;
    default_configuration.set_frequency(30.0);
    return default_configuration;
  }

  device::mojom::ReportingMode GetReportingMode() override {
    return device::mojom::ReportingMode::ON_CHANGE;
  }

  double GetMaximumSupportedFrequency() override { return 50.0; }
  double GetMinimumSupportedFrequency() override { return 1.0; }

 protected:
  ~FakeAmbientLightSensor() override = default;
};

class FakeSensorProvider : public device::PlatformSensorProvider {
 public:
  static FakeSensorProvider* GetInstance() {
    return base::Singleton<FakeSensorProvider, base::LeakySingletonTraits<
                                                   FakeSensorProvider>>::get();
  }

  FakeSensorProvider() = default;
  ~FakeSensorProvider() override = default;

  void CreateSensorInternal(device::mojom::SensorType type,
                            mojo::ScopedSharedBufferMapping mapping,
                            const CreateSensorCallback& callback) override {
    switch (type) {
      case device::mojom::SensorType::AMBIENT_LIGHT: {
        scoped_refptr<device::PlatformSensor> sensor =
            new FakeAmbientLightSensor(type, std::move(mapping), this);
        callback.Run(std::move(sensor));
        break;
      }
      default:
        NOTIMPLEMENTED();
        callback.Run(nullptr);
    }
  }
};

class GenericSensorServiceTest : public DeviceServiceTestBase,
                                 public device::mojom::SensorClient {
 public:
  GenericSensorServiceTest()
      : client_binding_(this),
        io_thread_task_runner_(io_thread_.task_runner()),
        io_loop_finished_event_(
            base::WaitableEvent::ResetPolicy::AUTOMATIC,
            base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kGenericSensor);
    DeviceServiceTestBase::SetUp();
    io_thread_task_runner_->PostTask(
        FROM_HERE, base::Bind(&GenericSensorServiceTest::SetUpOnIOThread,
                              base::Unretained(this)));
    io_loop_finished_event_.Wait();

    connector()->BindInterface(mojom::kServiceName, &sensor_provider_);
  }

  void TearDown() override {
    device::PlatformSensorProvider::SetProviderForTesting(nullptr);
  }

  void SetUpOnIOThread() {
    device::PlatformSensorProvider::SetProviderForTesting(
        FakeSensorProvider::GetInstance());
    io_loop_finished_event_.Signal();
  }

  // Implements device::mojom::SensorClient:
  void SensorReadingChanged() override {
    UpdateReadingData();
    check_value_.Run(GetReadingValue());
    run_loop_->Quit();
  }
  void RaiseError() override {}

  void OnSensorCreated(base::Closure quit_closure,
                       mojom::SensorInitParamsPtr params,
                       mojom::SensorClientRequest client_request) {
    EXPECT_TRUE(params);
    EXPECT_TRUE(params->memory.is_valid());
    EXPECT_TRUE(params->mode == mojom::ReportingMode::ON_CHANGE);
    EXPECT_TRUE(params->buffer_offset ==
                ((static_cast<uint64_t>(mojom::SensorType::LAST) -
                  static_cast<uint64_t>(mojom::SensorType::AMBIENT_LIGHT)) *
                 sizeof(SensorReadingSharedBuffer)));
    EXPECT_TRUE(params->default_configuration.frequency() == 30.0);
    EXPECT_TRUE(params->maximum_frequency == 50.0);
    EXPECT_TRUE(params->minimum_frequency == 1.0);

    shared_buffer_ = params->memory->MapAtOffset(
        mojom::SensorInitParams::kReadBufferSizeForTests,
        params->buffer_offset);

    client_binding_.Bind(std::move(client_request));
    quit_closure.Run();
  }

  void OnGetDefaultConfiguration(
      base::Closure quit_closure,
      const device::PlatformSensorConfiguration& configuration) {
    EXPECT_TRUE(configuration.frequency() == 30.0);
    quit_closure.Run();
  }

  void OnAddOrRemoveConfiguration(base::Callback<void(bool)> expect_function,
                                  bool is_success) {
    expect_function.Run(is_success);
  }

  device::mojom::SensorProviderPtr sensor_provider_;
  device::mojom::SensorPtr sensor_;

  // Use |run_loop_| to guarantee SensorReadingChanged() does be called before
  // the main thread exits, use |check_value_| to verify the data is same as
  // expected in SensorReadingChanged().
  std::unique_ptr<base::RunLoop> run_loop_;
  base::Callback<void(double)> check_value_;

 private:
  double GetReadingValue() { return reading_data_.values[0]; }

  void UpdateReadingData() {
    memset(&reading_data_, 0, sizeof(device::SensorReading));
    int read_attempts = 0;
    const int kMaxReadAttemptsCount = 10;
    while (!TryReadFromBuffer(reading_data_)) {
      if (++read_attempts == kMaxReadAttemptsCount) {
        EXPECT_FALSE(1);
        return;
      }
    }
  }

  bool TryReadFromBuffer(device::SensorReading& result) {
    const SensorReadingSharedBuffer* buffer =
        static_cast<const SensorReadingSharedBuffer*>(shared_buffer_.get());
    const device::OneWriterSeqLock& seqlock = buffer->seqlock.value();
    auto version = seqlock.ReadBegin();
    auto reading_data = buffer->reading;
    if (seqlock.ReadRetry(version))
      return false;
    result = reading_data;
    return true;
  }

  mojo::Binding<device::mojom::SensorClient> client_binding_;
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;
  base::WaitableEvent io_loop_finished_event_;

  mojo::ScopedSharedBufferMapping shared_buffer_;
  device::SensorReading reading_data_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(GenericSensorServiceTest);
};

// Requests the SensorProvider to create a sensor.
TEST_F(GenericSensorServiceTest, GetSensorTest) {
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), run_loop.QuitClosure()));

  run_loop.Run();
}

// Tests GetDefaultConfiguration.
TEST_F(GenericSensorServiceTest, GetDefaultConfigurationTest) {
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  sensor_->GetDefaultConfiguration(
      base::Bind(&GenericSensorServiceTest::OnGetDefaultConfiguration,
                 base::Unretained(this), run_loop.QuitClosure()));

  run_loop.Run();
}

// Tests adding a valid configuration. Client should be notified by
// SensorClient::SensorReadingChanged().
TEST_F(GenericSensorServiceTest, ValidAddConfigurationTest) {
  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  device::PlatformSensorConfiguration configuration(50.0);
  sensor_->AddConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, base::Bind(&DoNothing), true)));

  // Expect the SensorReadingChanged() will be called.
  check_value_ = base::Bind(&CheckValue, 50.0);
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();
}

// Tests adding an invalid configuation, the max allowed frequency is 50.0 in
// the mocked SensorImpl, while we add one with 60.0.
TEST_F(GenericSensorServiceTest, InvalidAddConfigurationTest) {
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  // Invalid configuration that exceeds the max allowed frequency.
  device::PlatformSensorConfiguration configuration(60.0);
  sensor_->AddConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), false)));

  run_loop.Run();
}

// Adds a valid configuration, then remove it.
TEST_F(GenericSensorServiceTest, ValidRemoveConfigurationTest) {
  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  device::PlatformSensorConfiguration configuration(50.0);
  sensor_->AddConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, base::Bind(&DoNothing), true)));

  // Expect the SensorReadingChanged() will be called.
  check_value_ = base::Bind(&CheckValue, 50.0);
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();

  base::RunLoop run_loop;
  sensor_->RemoveConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), true)));
  run_loop.Run();
}

// Removes an invalid configuration.
TEST_F(GenericSensorServiceTest, InvalidRemoveConfigurationTest) {
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  device::PlatformSensorConfiguration configuration(50.0);
  sensor_->RemoveConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), false)));

  run_loop.Run();
}

// Test suspend. After suspending, the client won't be notified by
// SensorReadingChanged() after calling AddConfiguration.
// Call the AddConfiguration() twice, if the SensorReadingChanged() was
// called, it should happen before the callback triggerred by the second
// AddConfiguration(). In this way we make sure it won't be missed by the
// early quit of main thread (if there is an unexpected notification by
// SensorReadingChanged()).
TEST_F(GenericSensorServiceTest, SuspendTest) {
  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  sensor_->Suspend();
  // Expect the SensorReadingChanged() will not be called. Pass a bad value to
  // |check_value_| to guarantee SensorReadingChanged() really doesn't be
  // called. Otherwise the CheckValue() will complain the bad value.
  check_value_ = base::Bind(&CheckValue, 123.0);

  base::RunLoop run_loop;
  device::PlatformSensorConfiguration configuration(30.0);
  sensor_->AddConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, base::Bind(&DoNothing), true)));
  device::PlatformSensorConfiguration configuration_2(31.0);
  sensor_->AddConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), true)));
  run_loop.Run();
}

// Test suspend and resume. After resuming, client can add configuration and
// be notified by SensorReadingChanged() as usual.
TEST_F(GenericSensorServiceTest, SuspendThenResumeTest) {
  sensor_provider_->GetSensor(
      device::mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_),
      base::Bind(&GenericSensorServiceTest::OnSensorCreated,
                 base::Unretained(this), base::Bind(&DoNothing)));

  device::PlatformSensorConfiguration configuration(30.0);
  sensor_->AddConfiguration(
      configuration,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, base::Bind(&DoNothing), true)));

  // Expect the SensorReadingChanged() will be called.
  check_value_ = base::Bind(&CheckValue, 30.0);
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();

  sensor_->Suspend();
  sensor_->Resume();

  device::PlatformSensorConfiguration configuration_2(50.0);
  sensor_->AddConfiguration(
      configuration_2,
      base::Bind(&GenericSensorServiceTest::OnAddOrRemoveConfiguration,
                 base::Unretained(this),
                 base::Bind(&CheckSuccess, base::Bind(&DoNothing), true)));

  // Expect the SensorReadingChanged() will be called.
  check_value_ = base::Bind(&CheckValue, 50.0);
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();
}

}  //  namespace

}  //  namespace device
