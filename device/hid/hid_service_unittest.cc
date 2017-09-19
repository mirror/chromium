// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/hid_service.h"
#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "build/build_config.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

class HidServiceTest : public ::testing::Test {
 public:
  HidServiceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

void OnGetDevices(const base::Closure& quit_closure,
                  std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  // Since there's no guarantee that any devices are connected at the moment
  // this test doesn't assume anything about the result but it at least verifies
  // that devices can be enumerated without the application crashing.
  quit_closure.Run();
}

}  // namespace

TEST_F(HidServiceTest, GetDevices) {
  HidService* service = nullptr;

// The HID service is not available on all platforms.
#if !defined(OS_ANDROID) && !defined(OS_IOS) && \
    !(defined(OS_LINUX) && !defined(USE_UDEV))
  service = HidService::Create();
#endif

  if (service) {
    base::RunLoop loop;
    service->GetDevices(base::Bind(&OnGetDevices, loop.QuitClosure()));
    loop.Run();
  }
}

}  // namespace device
