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

}  // namespace

namespace multidevice {

template <typename... Args>
void IgnoreAllArgs(Args&&...) {}

template <typename... Args>
void DoCaptures(typename std::decay<Args>::type*... out_args,
                const base::Closure& quit_closure,
                Args... in_args) {
  IgnoreAllArgs((*out_args = std::move(in_args))...);
  quit_closure.Run();
}

template <typename T1>
base::Callback<void(T1)> Capture(T1* t1, const base::Closure& quit_closure) {
  return base::Bind(&DoCaptures<T1>, t1, quit_closure);
}

class MultiDeviceServiceTest : public service_manager::test::ServiceTest {
 public:
  class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver {
   public:
    DeviceSyncObserverImpl(
        device_sync::mojom::DeviceSyncObserverRequest request)
        : binding_(this, std::move(request)) {}

    void OnEnrollmentFinished(bool success) override {
      if (success && result_code_ == device_sync::mojom::ResultCode::SUCCESS) {
        num_times_enrollment_finished_called_++;
      }
      std::move(Capture(&error_code_, run_loop_->QuitClosure()))
          .Run(result_code_);
    }

    void OnDevicesSynced(bool success) override {
      if (success && result_code_ == device_sync::mojom::ResultCode::SUCCESS) {
        num_times_device_synced_++;
      }
      std::move(Capture(&error_code_, run_loop_->QuitClosure()))
          .Run(result_code_);
    }

    void SetExpectedResultCode(device_sync::mojom::ResultCode result_code) {
      result_code_ = result_code;
    }

    int NumTimesOnEnrollmentFinishedCalled() {
      return num_times_enrollment_finished_called_;
    }

    int NumTimesOnDevicesSyncedCalled() { return num_times_device_synced_; }

    std::unique_ptr<base::RunLoop> run_loop_;

   private:
    int num_times_device_synced_ = 0;
    int num_times_enrollment_finished_called_ = 0;
    mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;
    device_sync::mojom::ResultCode error_code_;
    device_sync::mojom::ResultCode result_code_ =
        device_sync::mojom::ResultCode::SUCCESS;
  };

  MultiDeviceServiceTest() : ServiceTest(kServiceTestName){};

  ~MultiDeviceServiceTest() override{};

  void SetUp() override {
    ServiceTest::SetUp();
    ASSERT_FALSE(device_sync_ptr_);
    connector()->BindInterface(multidevice::mojom::kServiceName,
                               &device_sync_ptr_);
    ASSERT_TRUE(device_sync_ptr_);
  };

  void AddObservers() {
    for (auto& observer_ptrs_ : observer_ptrs_) {
      device_sync_ptr_->AddObserver(std::move(observer_ptrs_));
    }
  }

  void ForceEnrollmentNow() {
    for (const auto& observer : observers_) {
      observer->run_loop_.reset(new base::RunLoop());
    }
    device_sync_ptr_->ForceEnrollmentNow();
    for (const auto& observer : observers_) {
      observer->run_loop_->Run();
    }
  }

  void ForceSyncNow() {
    for (const auto& observer : observers_) {
      observer->run_loop_.reset(new base::RunLoop());
    }
    device_sync_ptr_->ForceSyncNow();
    for (const auto& observer : observers_) {
      observer->run_loop_->Run();
    }
  }

  void GenerateDeviceSyncObserver(int num) {
    for (int i = 0; i < num; i++) {
      observer_ptrs_.emplace_back(device_sync::mojom::DeviceSyncObserverPtr());
      observers_.emplace_back(base::MakeUnique<DeviceSyncObserverImpl>(
          mojo::MakeRequest(&observer_ptrs_[i])));
    }
  }

  device_sync::mojom::DeviceSyncPtr device_sync_ptr_;

  std::vector<std::unique_ptr<DeviceSyncObserverImpl>> observers_;
  std::vector<device_sync::mojom::DeviceSyncObserverPtr> observer_ptrs_;
};

TEST_F(MultiDeviceServiceTest, MultipleCallTest) {
  GenerateDeviceSyncObserver(2);
  AddObservers();

  ForceEnrollmentNow();
  EXPECT_EQ(observers_[0]->NumTimesOnEnrollmentFinishedCalled(), 1);
  EXPECT_EQ(observers_[1]->NumTimesOnEnrollmentFinishedCalled(), 1);

  ForceSyncNow();
  EXPECT_EQ(observers_[0]->NumTimesOnDevicesSyncedCalled(), 1);
  EXPECT_EQ(observers_[1]->NumTimesOnDevicesSyncedCalled(), 1);

  ForceEnrollmentNow();
  EXPECT_EQ(observers_[0]->NumTimesOnEnrollmentFinishedCalled(), 2);
  EXPECT_EQ(observers_[1]->NumTimesOnEnrollmentFinishedCalled(), 2);

  ForceSyncNow();
  EXPECT_EQ(observers_[0]->NumTimesOnDevicesSyncedCalled(), 2);
  EXPECT_EQ(observers_[1]->NumTimesOnDevicesSyncedCalled(), 2);
}

}  // namespace multidevice