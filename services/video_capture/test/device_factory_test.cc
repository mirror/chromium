// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/test/device_factory_test.h"

#include "base/command_line.h"
#include "media/base/media_switches.h"
#include "services/service_manager/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/interfaces/service_manager.mojom.h"
#include "services/video_capture/public/interfaces/constants.mojom.h"

namespace video_capture {

ServiceManagerListenerImpl::ServiceManagerListenerImpl(
    service_manager::mojom::ServiceManagerListenerRequest request,
    base::RunLoop* loop)
    : binding_(this, std::move(request)), loop_(loop) {}

ServiceManagerListenerImpl::~ServiceManagerListenerImpl() = default;

DeviceFactoryTest::DeviceFactoryTest()
    : service_manager::test::ServiceTest("video_capture_unittests") {}

DeviceFactoryTest::~DeviceFactoryTest() = default;

void DeviceFactoryTest::SetUp() {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kUseFakeDeviceForMediaStream);

  service_manager::test::ServiceTest::SetUp();

  service_manager::mojom::ServiceManagerPtr service_manager;
  connector()->BindInterface(service_manager::mojom::kServiceName,
                             &service_manager);
  service_manager::mojom::ServiceManagerListenerPtr listener;
  base::RunLoop loop;
  service_state_observer_ = base::MakeUnique<ServiceManagerListenerImpl>(
      mojo::MakeRequest(&listener), &loop);
  service_manager->AddListener(std::move(listener));
  loop.Run();

  connector()->BindInterface(mojom::kServiceName, &service_);
  service_->SetShutdownDelayInSeconds(0.0f);
  service_->ConnectToDeviceFactory(mojo::MakeRequest(&factory_));
}

}  // namespace video_capture
