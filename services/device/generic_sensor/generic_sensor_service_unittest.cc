// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind_helpers.h"
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

class FakePlatformSensor : public PlatformSensor {
 public:
  FakePlatformSensor(mojom::SensorType type,
                     mojo::ScopedSharedBufferMapping mapping,
                     PlatformSensorProvider* provider)
      : PlatformSensor(type, std::move(mapping), provider) {}

  bool StartSensor(const PlatformSensorConfiguration& configuration) override {
    SensorReading reading;
    // Only mocking the shared memory update for AMBIENT_LIGHT type is enough.
    if (GetType() == mojom::SensorType::AMBIENT_LIGHT) {
      // Set the shared buffer value as frequency for testing purpose.
      reading.values[0] = configuration.frequency();
      UpdateSensorReading(reading, true);
    }
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
    // Set the ReportingMode as ON_CHANGE, so we can test the
    // SensorReadingChanged() mojo interface.
    return mojom::ReportingMode::ON_CHANGE;
  }

  double GetMaximumSupportedFrequency() override { return 50.0; }
  double GetMinimumSupportedFrequency() override { return 1.0; }

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

  void CreateSensorInternal(mojom::SensorType type,
                            mojo::ScopedSharedBufferMapping mapping,
                            const CreateSensorCallback& callback) override {
    DCHECK(type >= mojom::SensorType::FIRST && type <= mojom::SensorType::LAST);
    scoped_refptr<FakePlatformSensor> sensor =
        new FakePlatformSensor(type, std::move(mapping), this);
    callback.Run(sensor);
  }

  DISALLOW_COPY_AND_ASSIGN(FakePlatformSensorProvider);
};

class FakeSensorClient : public mojom::SensorClient {
 public:
  FakeSensorClient() : client_binding_(this) {}

  // Implements mojom::SensorClient:
  void SensorReadingChanged() override {
    UpdateReadingData();
    check_value_.Run(GetReadingValue());
    quit_closure_.Run();
    quit_closure_ = base::Bind(&base::DoNothing);
  }
  void RaiseError() override {}

  // Sensor mojo interfaces callbacks:
  void OnSensorCreated(base::Closure quit_closure,
                       mojom::SensorInitParamsPtr params,
                       mojom::SensorClientRequest client_request) {
    EXPECT_TRUE(params);
    EXPECT_TRUE(params->memory.is_valid());
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
      const PlatformSensorConfiguration& configuration) {
    EXPECT_TRUE(configuration.frequency() == 30.0);
    quit_closure.Run();
  }

  void OnAddOrRemoveConfiguration(base::Callback<void(bool)> expect_function,
                                  bool is_success) {
    expect_function.Run(is_success);
  }

  // For SensorReadingChanged().
  void SetQuitClosure(base::Closure quit_closure) {
    quit_closure_ = quit_closure;
  }

  void SetCheckValueCallback(base::Callback<void(double)> callback) {
    check_value_ = callback;
  }

 private:
  double GetReadingValue() { return reading_data_.values[0]; }

  void UpdateReadingData() {
    memset(&reading_data_, 0, sizeof(SensorReading));
    int read_attempts = 0;
    const int kMaxReadAttemptsCount = 10;
    while (!TryReadFromBuffer(reading_data_)) {
      if (++read_attempts == kMaxReadAttemptsCount) {
        EXPECT_FALSE(1);
        return;
      }
    }
  }

  bool TryReadFromBuffer(SensorReading& result) {
    const SensorReadingSharedBuffer* buffer =
        static_cast<const SensorReadingSharedBuffer*>(shared_buffer_.get());
    const OneWriterSeqLock& seqlock = buffer->seqlock.value();
    auto version = seqlock.ReadBegin();
    auto reading_data = buffer->reading;
    if (seqlock.ReadRetry(version))
      return false;
    result = reading_data;
    return true;
  }

  mojo::Binding<mojom::SensorClient> client_binding_;
  mojo::ScopedSharedBufferMapping shared_buffer_;
  SensorReading reading_data_;

  // Set the quit-closure that will be run by SensorReadingChanged(), and
  // runs a RunLoop in main thread. In this way we guarantee the
  // SensorReadingChanged() does be triggered.
  base::Closure quit_closure_;

  // Uses |check_value_| to verify the data is same as we
  // expected in SensorReadingChanged().
  base::Callback<void(double)> check_value_;

  DISALLOW_COPY_AND_ASSIGN(FakeSensorClient);
};

class GenericSensorServiceTest : public DeviceServiceTestBase {
 public:
  GenericSensorServiceTest()
      : io_thread_task_runner_(io_thread_.task_runner()),
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
    PlatformSensorProvider::SetProviderForTesting(nullptr);
  }

  void SetUpOnIOThread() {
    PlatformSensorProvider::SetProviderForTesting(
        FakePlatformSensorProvider::GetInstance());
    io_loop_finished_event_.Signal();
  }

  mojom::SensorProviderPtr sensor_provider_;
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;
  base::WaitableEvent io_loop_finished_event_;
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(GenericSensorServiceTest);
};

// Requests the SensorProvider to create a sensor.
TEST_F(GenericSensorServiceTest, GetSensorTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  base::RunLoop run_loop;
  sensor_provider_->GetSensor(
      mojom::SensorType::PROXIMITY, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), run_loop.QuitClosure()));
  run_loop.Run();
}

// Tests GetDefaultConfiguration.
TEST_F(GenericSensorServiceTest, GetDefaultConfigurationTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      mojom::SensorType::ACCELEROMETER, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  sensor->GetDefaultConfiguration(
      base::Bind(&FakeSensorClient::OnGetDefaultConfiguration,
                 base::Unretained(client.get()), run_loop.QuitClosure()));

  run_loop.Run();
}

// Tests adding a valid configuration. Client should be notified by
// SensorClient::SensorReadingChanged().
TEST_F(GenericSensorServiceTest, ValidAddConfigurationTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  PlatformSensorConfiguration configuration(50.0);
  sensor->AddConfiguration(
      configuration,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));

  // Expect the SensorReadingChanged() will be called after AddConfiguration.
  base::RunLoop run_loop;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 50.0));
  client->SetQuitClosure(run_loop.QuitClosure());
  run_loop.Run();
}

// Tests adding an invalid configuation, the max allowed frequency is 50.0 in
// the mocked SensorImpl, while we add one with 60.0.
TEST_F(GenericSensorServiceTest, InvalidAddConfigurationTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      mojom::SensorType::LINEAR_ACCELERATION, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  // Invalid configuration that exceeds the max allowed frequency.
  PlatformSensorConfiguration configuration(60.0);
  sensor->AddConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client.get()),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), false)));

  run_loop.Run();
}

// Tests adding more than one clients. Sensor should send notification to all
// its clients.
TEST_F(GenericSensorServiceTest, MultipleClientsTest) {
  base::RunLoop run_loop;
  mojom::SensorPtr sensor_1;
  std::unique_ptr<FakeSensorClient> client_1 =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(mojom::SensorType::AMBIENT_LIGHT,
                              mojo::MakeRequest(&sensor_1),
                              base::Bind(&FakeSensorClient::OnSensorCreated,
                                         base::Unretained(client_1.get()),
                                         base::Bind(&base::DoNothing)));

  mojom::SensorPtr sensor_2;
  std::unique_ptr<FakeSensorClient> client_2 =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(mojom::SensorType::AMBIENT_LIGHT,
                              mojo::MakeRequest(&sensor_2),
                              base::Bind(&FakeSensorClient::OnSensorCreated,
                                         base::Unretained(client_2.get()),
                                         base::Bind(&base::DoNothing)));

  PlatformSensorConfiguration configuration(48.0);
  sensor_1->AddConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client_1.get()),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), true)));
  run_loop.Run();

  // Expect the SensorReadingChanged() will be called for both clients.
  base::RunLoop run_loop_1;
  client_1->SetCheckValueCallback(base::Bind(&CheckValue, 48.0));
  client_1->SetQuitClosure(run_loop_1.QuitClosure());
  run_loop_1.Run();

  base::RunLoop run_loop_2;
  client_2->SetCheckValueCallback(base::Bind(&CheckValue, 48.0));
  client_2->SetQuitClosure(run_loop_2.QuitClosure());
  run_loop_2.Run();
}

// Tests adding more than one clients. If mojo connection is broken on one
// client, other clients should not be affected.
TEST_F(GenericSensorServiceTest, ClientMojoConnectionBrokenTest) {
  base::RunLoop run_loop_1;
  mojom::SensorPtr sensor_1;
  std::unique_ptr<FakeSensorClient> client_1 =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(mojom::SensorType::AMBIENT_LIGHT,
                              mojo::MakeRequest(&sensor_1),
                              base::Bind(&FakeSensorClient::OnSensorCreated,
                                         base::Unretained(client_1.get()),
                                         base::Bind(&base::DoNothing)));

  mojom::SensorPtr sensor_2;
  std::unique_ptr<FakeSensorClient> client_2 =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor_2),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client_2.get()), run_loop_1.QuitClosure()));
  run_loop_1.Run();

  // Breaks mojo connection of client_1.
  sensor_1.reset();

  base::RunLoop run_loop_2;
  PlatformSensorConfiguration configuration(48.0);
  sensor_2->AddConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client_2.get()),
                 base::Bind(&CheckSuccess, run_loop_2.QuitClosure(), true)));
  run_loop_2.Run();

  // Expect the SensorReadingChanged() will be called on client_2.
  base::RunLoop run_loop_3;
  client_2->SetCheckValueCallback(base::Bind(&CheckValue, 48.0));
  client_2->SetQuitClosure(run_loop_3.QuitClosure());
  run_loop_3.Run();
}

// Adds a valid configuration, then remove it.
TEST_F(GenericSensorServiceTest, ValidRemoveConfigurationTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  PlatformSensorConfiguration configuration(50.0);
  sensor->AddConfiguration(
      configuration,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));

  // Expect the SensorReadingChanged() will be called after AddConfiguration.
  base::RunLoop run_loop_1;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 50.0));
  client->SetQuitClosure(run_loop_1.QuitClosure());
  run_loop_1.Run();

  base::RunLoop run_loop_2;
  sensor->RemoveConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client.get()),
                 base::Bind(&CheckSuccess, run_loop_2.QuitClosure(), true)));
  run_loop_2.Run();
}

// Removes an invalid configuration.
TEST_F(GenericSensorServiceTest, InvalidRemoveConfigurationTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  base::RunLoop run_loop;

  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  PlatformSensorConfiguration configuration(50.0);
  sensor->RemoveConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client.get()),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), false)));

  run_loop.Run();
}

// Test mixed add and remove configuration operations.
TEST_F(GenericSensorServiceTest, MixedAddAndRemoveConfigurationTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  // Expect the SensorReadingChanged() will be called. The frequency value
  // should be 30.0
  PlatformSensorConfiguration configuration_30(30.0);
  sensor->AddConfiguration(
      configuration_30,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));
  base::RunLoop run_loop_1;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 30.0));
  client->SetQuitClosure(run_loop_1.QuitClosure());
  run_loop_1.Run();

  // Expect the SensorReadingChanged() will be called. The frequency value
  // should be 30.0 instead of 20.0
  PlatformSensorConfiguration configuration_20(20.0);
  sensor->AddConfiguration(
      configuration_20,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));
  base::RunLoop run_loop_2;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 30.0));
  client->SetQuitClosure(run_loop_2.QuitClosure());
  run_loop_2.Run();

  // Expect the SensorReadingChanged() will be called. The frequency value
  // should be 20.0.
  sensor->RemoveConfiguration(
      configuration_30,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));
  base::RunLoop run_loop_3;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 20.0));
  client->SetQuitClosure(run_loop_3.QuitClosure());
  run_loop_3.Run();
}

// Test suspend. After suspending, the client won't be notified by
// SensorReadingChanged() after calling AddConfiguration.
// Call the AddConfiguration() twice, if the SensorReadingChanged() was
// called, it should happen before the callback triggerred by the second
// AddConfiguration(). In this way we make sure it won't be missed by the
// early quit of main thread (when there is an unexpected notification by
// SensorReadingChanged()).
TEST_F(GenericSensorServiceTest, SuspendTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  sensor->Suspend();

  // Expect the SensorReadingChanged() won't be called. Pass a bad value(123.0)
  // to |check_value_| to guarantee SensorReadingChanged() really doesn't be
  // called. Otherwise the CheckValue() will complain on the bad value.
  client->SetCheckValueCallback(base::Bind(&CheckValue, 123.0));

  base::RunLoop run_loop;
  PlatformSensorConfiguration configuration(30.0);
  sensor->AddConfiguration(
      configuration,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));
  PlatformSensorConfiguration configuration_2(31.0);
  sensor->AddConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client.get()),
                 base::Bind(&CheckSuccess, run_loop.QuitClosure(), true)));
  run_loop.Run();
}

// Test suspend and resume. After resuming, client can add configuration and
// be notified by SensorReadingChanged() as usual.
TEST_F(GenericSensorServiceTest, SuspendThenResumeTest) {
  mojom::SensorPtr sensor;
  std::unique_ptr<FakeSensorClient> client =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(
      mojom::SensorType::AMBIENT_LIGHT, mojo::MakeRequest(&sensor),
      base::Bind(&FakeSensorClient::OnSensorCreated,
                 base::Unretained(client.get()), base::Bind(&base::DoNothing)));

  // Expect the SensorReadingChanged() will be called. The frequency should
  // be 30.0 after AddConfiguration.
  PlatformSensorConfiguration configuration(30.0);
  sensor->AddConfiguration(
      configuration,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));
  base::RunLoop run_loop_1;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 30.0));
  client->SetQuitClosure(run_loop_1.QuitClosure());
  run_loop_1.Run();

  sensor->Suspend();
  sensor->Resume();

  // Expect the SensorReadingChanged() will be called. The frequency should
  // be 50.0 after new configuration is added.
  PlatformSensorConfiguration configuration_2(50.0);
  sensor->AddConfiguration(
      configuration_2,
      base::Bind(
          &FakeSensorClient::OnAddOrRemoveConfiguration,
          base::Unretained(client.get()),
          base::Bind(&CheckSuccess, base::Bind(&base::DoNothing), true)));
  base::RunLoop run_loop_2;
  client->SetCheckValueCallback(base::Bind(&CheckValue, 50.0));
  client->SetQuitClosure(run_loop_2.QuitClosure());
  run_loop_2.Run();
}

// Test suspend when there are more than one client. The suspended client won't
// receive SensorReadingChanged() notification.
TEST_F(GenericSensorServiceTest, MultipleClientsSuspendAndResumeTest) {
  mojom::SensorPtr sensor_1;
  std::unique_ptr<FakeSensorClient> client_1 =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(mojom::SensorType::AMBIENT_LIGHT,
                              mojo::MakeRequest(&sensor_1),
                              base::Bind(&FakeSensorClient::OnSensorCreated,
                                         base::Unretained(client_1.get()),
                                         base::Bind(&base::DoNothing)));

  mojom::SensorPtr sensor_2;
  std::unique_ptr<FakeSensorClient> client_2 =
      base::MakeUnique<FakeSensorClient>();
  sensor_provider_->GetSensor(mojom::SensorType::AMBIENT_LIGHT,
                              mojo::MakeRequest(&sensor_2),
                              base::Bind(&FakeSensorClient::OnSensorCreated,
                                         base::Unretained(client_2.get()),
                                         base::Bind(&base::DoNothing)));
  sensor_1->Suspend();

  base::RunLoop run_loop_1;
  PlatformSensorConfiguration configuration(46.0);
  sensor_2->AddConfiguration(
      configuration,
      base::Bind(&FakeSensorClient::OnAddOrRemoveConfiguration,
                 base::Unretained(client_2.get()),
                 base::Bind(&CheckSuccess, run_loop_1.QuitClosure(), true)));
  run_loop_1.Run();

  // Expect the sensor_2 will receive SensorReadingChanged() notification while
  // sensor_1 won't.
  base::RunLoop run_loop_2;
  client_2->SetCheckValueCallback(base::Bind(&CheckValue, 46.0));
  client_2->SetQuitClosure(run_loop_2.QuitClosure());
  run_loop_2.Run();
}

}  //  namespace

}  //  namespace device
