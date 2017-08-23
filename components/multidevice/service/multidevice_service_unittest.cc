// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// #include "multidevice_service.h"

#include <memory>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/multidevice/service/device_sync_impl.h"
#include "components/multidevice/service/multidevice_service.h"
#include "components/multidevice/service/public/interfaces/constants.mojom.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kServiceTestName[] = "multidevice_service_unittest";

struct success_failure_count {
  int success_count = 0;
  int failure_count = 0;
};

enum class MultiDeviceServiceActionType {
  FORCE_ENROLLMENT_NOW,
  FORCE_SYNC_NOW
};

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
      if (success) {
        num_times_enrollment_finished_called_.success_count++;
      } else {
        num_times_enrollment_finished_called_.failure_count++;
      }
      on_callback_invoked_->Run();
    }

    void OnDevicesSynced(bool success) override {
      if (success) {
        num_times_device_synced_.success_count++;
      } else {
        num_times_device_synced_.failure_count++;
      }
      on_callback_invoked_->Run();
    }

    // Sets the necessary callback that will be invoked upon each interface
    // method call in order to return control to the test.
    void SetOnCallbackInvokedClosure(base::Closure* closure) {
      on_callback_invoked_ = closure;
    }

    int NumTimesOnEnrollmentFinishedCalled(bool success_count) {
      return success_count
                 ? num_times_enrollment_finished_called_.success_count
                 : num_times_enrollment_finished_called_.failure_count;
    }

    int NumTimesOnDevicesSyncedCalled(bool success_count) {
      return success_count ? num_times_device_synced_.success_count
                           : num_times_device_synced_.failure_count;
    }

   private:
    mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;
    base::Closure* on_callback_invoked_ = nullptr;

    success_failure_count num_times_enrollment_finished_called_;
    success_failure_count num_times_device_synced_;
  };

  MultiDeviceServiceTest() : ServiceTest(kServiceTestName){};

  ~MultiDeviceServiceTest() override{};

  void SetUp() override {
    ServiceTest::SetUp();
    connector()->BindInterface(multidevice::mojom::kServiceName,
                               &device_sync_ptr_);
  }

  void AddDeviceSyncObservers(int num) {
    device_sync::mojom::DeviceSyncObserverPtr device_sync_observer_ptr;
    for (int i = 0; i < num; i++) {
      device_sync_observer_ptr = device_sync::mojom::DeviceSyncObserverPtr();
      observers_.emplace_back(base::MakeUnique<DeviceSyncObserverImpl>(
          mojo::MakeRequest(&device_sync_observer_ptr)));
      device_sync_ptr_->AddObserver(std::move(device_sync_observer_ptr));
    }
  }

  void MultDeviceServiceAction(MultiDeviceServiceActionType type) {
    base::RunLoop run_loop;
    base::Closure closure = base::BarrierClosure(
        static_cast<int>(observers_.size()), run_loop.QuitClosure());
    for (auto& observer : observers_) {
      observer->SetOnCallbackInvokedClosure(&closure);
    }

    switch (type) {
      case MultiDeviceServiceActionType::FORCE_ENROLLMENT_NOW:
        device_sync_ptr_->ForceEnrollmentNow();
        break;
      case MultiDeviceServiceActionType::FORCE_SYNC_NOW:
        device_sync_ptr_->ForceSyncNow();
        break;
      default:
        NOTREACHED();
    }

    run_loop.Run();
  }

  device_sync::mojom::DeviceSyncPtr device_sync_ptr_;
  std::vector<std::unique_ptr<DeviceSyncObserverImpl>> observers_;
};

TEST_F(MultiDeviceServiceTest, MultipleCallTest) {
  AddDeviceSyncObservers(2);

  MultDeviceServiceAction(MultiDeviceServiceActionType::FORCE_ENROLLMENT_NOW);
  EXPECT_EQ(observers_[0]->NumTimesOnEnrollmentFinishedCalled(
                true /* success_count */),
            1);
  EXPECT_EQ(observers_[1]->NumTimesOnEnrollmentFinishedCalled(
                true /* success_count */),
            1);
  EXPECT_EQ(observers_[0]->NumTimesOnEnrollmentFinishedCalled(
                false /* success_count */),
            0);
  EXPECT_EQ(observers_[1]->NumTimesOnEnrollmentFinishedCalled(
                false /* success_count */),
            0);

  MultDeviceServiceAction(MultiDeviceServiceActionType::FORCE_SYNC_NOW);
  EXPECT_EQ(
      observers_[0]->NumTimesOnDevicesSyncedCalled(true /* success_count */),
      1);
  EXPECT_EQ(
      observers_[1]->NumTimesOnDevicesSyncedCalled(true /* success_count */),
      1);
  EXPECT_EQ(
      observers_[0]->NumTimesOnDevicesSyncedCalled(false /* success_count */),
      0);
  EXPECT_EQ(
      observers_[1]->NumTimesOnDevicesSyncedCalled(false /* success_count */),
      0);

  MultDeviceServiceAction(MultiDeviceServiceActionType::FORCE_ENROLLMENT_NOW);
  EXPECT_EQ(observers_[0]->NumTimesOnEnrollmentFinishedCalled(
                true /* success_count */),
            2);
  EXPECT_EQ(observers_[1]->NumTimesOnEnrollmentFinishedCalled(
                true /* success_count */),
            2);
  EXPECT_EQ(observers_[0]->NumTimesOnEnrollmentFinishedCalled(
                false /* success_count */),
            0);
  EXPECT_EQ(observers_[1]->NumTimesOnEnrollmentFinishedCalled(
                false /* success_count */),
            0);

  MultDeviceServiceAction(MultiDeviceServiceActionType::FORCE_SYNC_NOW);
  EXPECT_EQ(
      observers_[0]->NumTimesOnDevicesSyncedCalled(true /* success_count */),
      2);
  EXPECT_EQ(
      observers_[1]->NumTimesOnDevicesSyncedCalled(true /* success_count */),
      2);
  EXPECT_EQ(
      observers_[0]->NumTimesOnDevicesSyncedCalled(false /* success_count */),
      0);
  EXPECT_EQ(
      observers_[1]->NumTimesOnDevicesSyncedCalled(false /* success_count */),
      0);
}

}  // namespace multidevice