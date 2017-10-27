// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/media_switches.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "services/video_capture/public/interfaces/constants.mojom.h"
#include "services/video_capture/public/interfaces/device_factory_provider.mojom.h"
#include "services/video_capture/service_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace video_capture {

using testing::Exactly;
using testing::_;
using testing::Invoke;
using testing::InvokeWithoutArgs;

// Test fixture that creates a video_capture::ServiceImpl and sets up a
// local service_manager::Connector through which client code can connect to it.
class DeviceFactoryProviderConnectorTest : public ::testing::Test {
 public:
  DeviceFactoryProviderConnectorTest() {}

  ~DeviceFactoryProviderConnectorTest() override {}

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kUseFakeDeviceForMediaStream);
    connector_factory_ =
        base::MakeUnique<service_manager::TestConnectorFactory>(&service_impl_);
    connector_ = connector_factory_->CreateConnector();
    service_impl_.OnStart();

    connector_->BindInterface(mojom::kServiceName, &factory_provider_);
    factory_provider_->SetShutdownDelayInSeconds(0.0f);
    factory_provider_->ConnectToDeviceFactory(mojo::MakeRequest(&factory_));
  }

 protected:
  ServiceImpl service_impl_;
  mojom::DeviceFactoryProviderPtr factory_provider_;
  mojom::DeviceFactoryPtr factory_;
  base::MockCallback<mojom::DeviceFactory::GetDeviceInfosCallback>
      device_info_receiver_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
};

TEST_F(DeviceFactoryProviderConnectorTest, GetDeviceInfosCallbackArrives) {
  base::RunLoop wait_loop;
  EXPECT_CALL(device_info_receiver_, Run(_))
      .Times(Exactly(1))
      .WillOnce(InvokeWithoutArgs([&wait_loop]() { wait_loop.Quit(); }));

  factory_->GetDeviceInfos(device_info_receiver_.Get());
  wait_loop.Run();
}

TEST_F(DeviceFactoryProviderConnectorTest, ExampleTest) {
  service_impl_.SetFactoryProviderClientDisconnectedObserver(
      base::BindRepeating(
          []() { LOG(INFO) << "Client has disconnected from service"; }));
}

}  // namespace video_capture
