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
//#include "mojo/public/cpp/bindings/interface_request.h"
//#include "services/service_manager/public/cpp/service_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// TODO(hsuregan): Many of the services have a 'kServiceName' specified in the
// generated files -- should DeviceSync have a "kSubservice" name? Used to bind
// interface on setup
const char kDeviceSyncService[] = "Device Sync";

// class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver
// {
//  public:
//   DeviceSyncObserverImpl(std::string name) : name_(name){};

//   void OnEnrollmentFinished(bool success) override {
//     if (success) {
//       num_times_enrollment_finished_called_++;
//     }
//   }

//   int numTimesEnrollmentFinishedCalled() {
//     return num_times_enrollment_finished_called_;
//   }

//  private:
//   std::string name_;
//   int num_times_enrollment_finished_called_ = 0;
// };

// std::vector<DeviceSyncObserverImpl> GenerateObservers(int nObservers) {
//   std::vector<DeviceSyncObserverImpl> observers;
//   for (int i = 0; i < nObservers; i++) {
//     observers.push_back(DeviceSyncObserverImpl("testObserverNum" +
//     std::to_string(i)));
//   }
//   return observers;
// }
}  // namespace

namespace multidevice {

class MultiDeviceServiceTest : public service_manager::test::ServiceTest {
 public:
  class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver {
   public:
    explicit DeviceSyncObserverImpl(std::string name)
        : name_(name) {}  //, binding_(this) {};

    void OnEnrollmentFinished(bool success) override {
      if (success) {
        num_times_enrollment_finished_called_++;
      }
    }

    int numTimesEnrollmentFinishedCalled() {
      return num_times_enrollment_finished_called_;
    }

   private:
    std::string name_;
    int num_times_enrollment_finished_called_ = 0;
    // mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;
  };

  MultiDeviceServiceTest()
      : ServiceTest("multi_device_unittest",
                    false,
                    base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT){
            // MultiDeviceService ugh;
            // multi_device_service_ = base::MakeUnique<MultiDeviceService>();
            // for(int i = 0; i < 5; i++) {
            // 	device_sync_observers_.push_back(device_sync::mojom::DeviceSyncObserverPtr());
            // 	auto it = device_sync_observers_.end();
            // 	connector()->BindInterface(kDeviceSyncService, &(*it));
            // }
        };

  ~MultiDeviceServiceTest() override{};

 protected:
  void SetUp() override {
    LOG(ERROR) << "HEY YOU WHATSUP";
    ServiceTest::SetUp();
    LOG(ERROR) << kDeviceSyncService;
    connector()->BindInterface(kDeviceSyncService, &device_sync_ptr_);
    run_loop_ = base::MakeUnique<base::RunLoop>();
    // DeviceSyncImpl device_sync_impl;//mojo::MakeRequest(&device_sync_ptr_));
    // device_sync::mojom::DeviceSyncObserverPtr observer1;//, observer2;
    // DeviceSyncObserverImpl ugh = DeviceSyncObserverImpl("PLS WORK");
    // observer1 = ugh;

    // ASSERT_FALSE(poop);
    // connector()->BindInterface(kDeviceSyncService, &device_sync_ptr_);
    // ASSERT_TRUE(device_sync_ptr_);

    //  DeviceSyncObserverImpl test_observer(mojo::MakeRequest(&poop));

    // device_sync_ptr_->AddObserver(observer1);
    // device_sync_ptr_->AddObserver(observer2);
    // for (auto observer : device_sync_observers_) {
    //   device_sync_ptr_->AddObserver(observer);
    // }
  };

  std::unique_ptr<base::RunLoop> run_loop_;

  std::unique_ptr<MultiDeviceService> multi_device_service_;

  device_sync::mojom::DeviceSyncPtr device_sync_ptr_;
  std::vector<device_sync::mojom::DeviceSyncObserver> device_sync_observers_;
  device_sync::mojom::DeviceSyncObserverPtr myPtr;
};

TEST_F(MultiDeviceServiceTest, ForceEnrollmentNowSyncTest) {
  EXPECT_EQ(1, 0);
  // EXPECT_EQ(0, device_sync_observers_[0].numTimesEnrollmentFinishedCalled());
  // device_sync_ptr_->OnEnrollmentFinished(true);
  // EXPECT_EQ(1, device_sync_observers_[0].numTimesEnrollmentFinishedCalled());
  // device_sync_ptr_->OnEnrollmentFinished(true);
  // EXPECT_EQ(2, device_sync_observers_[0].numTimesEnrollmentFinishedCalled());
}

}  // namespace multidevice