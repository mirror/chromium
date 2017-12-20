// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/embedder/embedded_instance_manager.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "services/service_manager/embedder/embedded_service_info.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace service_manager {
namespace {

// void OnQuit(bool* quit_called) {
//   *quit_called = true;
// }

// std::unique_ptr<Service> CreateTestService() {
//   return std::make_unique<Service>();
// }

}  // namespace

// class EmbeddedInstanceManagerTestApi {
//  public:
//   EmbeddedInstanceManagerTestApi(EmbeddedInstanceManager* instance)
//       : instance_(instance) {}

//   base::Thread* GetThread() { return instance_->service_thread_.get(); }

//  private:
//   EmbeddedInstanceManager* instance_;

//   DISALLOW_COPY_AND_ASSIGN(EmbeddedInstanceManagerTestApi);
// };

// Required test cases:
// * Dedicated thread for service, different from owner thread
// * Service task runner given explicitly and happens to be same as owner
// thread, does not allow blocking * Service task runner not given (will
// fallback to use owner thread) * Service task runner given and different from
// owner thread.

// TEST(EmbeddedInstanceManager, ShutdownWaitsForThreadToQuit) {
//   base::test::ScopedTaskEnvironment scoped_task_environment;
//   EmbeddedServiceInfo embedded_service_info;
//   embedded_service_info.use_own_thread = true;
//   embedded_service_info.factory = base::BindOnce(&CreateTestService);
//   bool quit_called = false;
//   scoped_refptr<EmbeddedInstanceManager> instance_manager =
//       new EmbeddedInstanceManager("test", embedded_service_info,
//                                   base::BindOnce(&OnQuit, &quit_called));
//   instance_manager->BindServiceRequest(nullptr);
//   EmbeddedInstanceManagerTestApi test_api(instance_manager.get());
//   ASSERT_TRUE(test_api.GetThread());
//   scoped_refptr<base::SingleThreadTaskRunner> thread_task_runner =
//       test_api.GetThread()->task_runner();
//   instance_manager->ShutDown();
//   EXPECT_FALSE(test_api.GetThread());
//   // Further verification the thread was shutdown.
//   EXPECT_FALSE(
//       thread_task_runner->PostTask(FROM_HERE,
//       base::BindOnce(&base::DoNothing)));
//   // Because Shutdown() was explicitly called with the thread running the
//   // quit closure should not have run (yet, because the invocation was posted
//   // to the end of the current message queue).
//   // Q: Do we actually want it to not be called at all?
//   EXPECT_FALSE(quit_called);
//   scoped_task_environment.RunUntilIdle();
//   EXPECT_TRUE(quit_called);
// }

}  // namespace service_manager
