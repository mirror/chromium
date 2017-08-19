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

// std::unique_ptr<base::RunLoop> run_loop_;

class MultiDeviceServiceTest : public service_manager::test::ServiceTest {
 public:
  class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver {
   public:
    DeviceSyncObserverImpl(
        device_sync::mojom::DeviceSyncObserverRequest request)
        : binding_(this, std::move(request)) {}

    void OnEnrollmentFinished(bool success) override {
      device_sync::mojom::ResultCode error_code;
      auto callback = Capture(&error_code, run_loop_->QuitClosure());
      if (success) {
        num_times_enrollment_finished_called_++;
      }
      std::move(callback).Run(device_sync::mojom::ResultCode::SUCCESS);
    }

    int numTimesEnrollmentFinishedCalled() {
      return num_times_enrollment_finished_called_;
    }

    std::unique_ptr<base::RunLoop> run_loop_;

   private:
    int num_times_enrollment_finished_called_ = 0;
    mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;
  };

  MultiDeviceServiceTest() : ServiceTest(kServiceTestName){};

  ~MultiDeviceServiceTest() override{};

  void ForceEnrollmentNow(DeviceSyncObserverImpl& impl) {
    impl.run_loop_.reset(new base::RunLoop());
    device_sync_ptr_->ForceEnrollmentNow();
    impl.run_loop_->Run();
  }

  void SetUp() override {
    ServiceTest::SetUp();
    LOG(ERROR) << "constructing device_sync_impl";
    ASSERT_FALSE(device_sync_ptr_);
    connector()->BindInterface(multidevice::mojom::kServiceName,
                               &device_sync_ptr_);
    ASSERT_TRUE(device_sync_ptr_);
    // run_loop_ = base::MakeUnique<base::RunLoop>();
  };

  device_sync::mojom::DeviceSyncPtr device_sync_ptr_;
  std::vector<device_sync::mojom::DeviceSyncObserver> device_sync_observers_;
  device_sync::mojom::DeviceSyncObserverPtr single_observer_;
};

TEST_F(MultiDeviceServiceTest, ForceEnrollmentNowSyncTest) {
  LOG(ERROR) << "BP 1";
  DeviceSyncObserverImpl impl(mojo::MakeRequest(&single_observer_));
  LOG(ERROR) << "BP 2";
  device_sync_ptr_->AddObserver(std::move(single_observer_));
  LOG(ERROR) << "BP 3";
  ForceEnrollmentNow(impl);
  LOG(ERROR) << "BP 4";
  EXPECT_EQ(impl.numTimesEnrollmentFinishedCalled(), 1);
  ForceEnrollmentNow(impl);
  EXPECT_EQ(impl.numTimesEnrollmentFinishedCalled(), 2);
}

}  // namespace multidevice