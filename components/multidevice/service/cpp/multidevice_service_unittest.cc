// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// #include "multidevice_service.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/multidevice/service/cpp/device_sync_impl.h"
#include "components/multidevice/service/cpp/multidevice_service.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// TODO(hsuregan): Many of the services have a 'kServiceName' specified in the
// generated files -- should DeviceSync have a "kSubservice" name? Used to bind
// interface on setup
const char kServiceTestName[] = "multidevice_service_unittest";

}  // namespace

namespace multidevice {

class MultiDeviceServiceTest : public service_manager::test::ServiceTest {
 public:
  class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver {
   public:
    DeviceSyncObserverImpl(
        device_sync::mojom::DeviceSyncObserverRequest request)
        : binding_(this, std::move(request)) {}

    void OnEnrollmentFinished(bool success) override {
      LOG(ERROR) << "On enrollment called.";
      if (success) {
        num_times_enrollment_finished_called_++;
      }
    }

    int numTimesEnrollmentFinishedCalled() {
      return num_times_enrollment_finished_called_;
    }

   private:
    int num_times_enrollment_finished_called_ = 0;
    mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;
  };

  MultiDeviceServiceTest()
      : ServiceTest(
            kServiceTestName,
            false,
            base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT){};

  ~MultiDeviceServiceTest() override{};

 protected:
  void SetUp() override {
    ServiceTest::SetUp();

    ASSERT_FALSE(device_sync_ptr_);
    connector()->BindInterface(multidevice::mojom::kServiceName,
                               &device_sync_ptr_);
    ASSERT_TRUE(device_sync_ptr_);

    run_loop_ = base::MakeUnique<base::RunLoop>();
  };

  std::unique_ptr<base::RunLoop> run_loop_;
  device_sync::mojom::DeviceSyncPtr device_sync_ptr_;
  std::vector<device_sync::mojom::DeviceSyncObserver> device_sync_observers_;
  device_sync::mojom::DeviceSyncObserverPtr single_observer_;
};

TEST_F(MultiDeviceServiceTest, ForceEnrollmentNowSyncTest) {
  DeviceSyncObserverImpl impl(mojo::MakeRequest(&single_observer_));

  // connector()->BindInterface(kDeviceSyncService, &single_observer_);
  device_sync_ptr_->AddObserver(std::move(single_observer_));
  // run_loop_->RunUntilIdle();
  device_sync_ptr_->ForceEnrollmentNow();
  // run_loop_->RunUntilIdle();
  EXPECT_EQ(impl.numTimesEnrollmentFinishedCalled(), 1);
}

}  // namespace multidevice