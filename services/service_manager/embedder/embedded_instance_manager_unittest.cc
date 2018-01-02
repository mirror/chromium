// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/embedder/embedded_instance_manager.h"

#include <memory>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "services/service_manager/embedder/embedded_service_info.h"
#include "services/service_manager/public/interfaces/service.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Invoke;

namespace service_manager {

namespace {

class MockService : public Service {
 public:
  MOCK_METHOD0(OnStart, void());
  MOCK_METHOD3(DoOnBindInterface,
               void(const service_manager::BindSourceInfo& source_info,
                    const std::string& interface_name,
                    mojo::ScopedMessagePipeHandle& interface_pipe));
  MOCK_METHOD0(OnServiceManagerConnectionLost, bool());
  MOCK_METHOD0(OnContextHasBeenSet, void());

  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    DoOnBindInterface(source_info, interface_name, interface_pipe);
  }

  void InvokeContexQuitNow() { context()->QuitNow(); }
};

static const std::string kArbitraryServiceName = "test_service";
static const std::string kArbitraryThreadName = "test_thread";

}  // namespace

class EmbeddedInstanceManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    testcase_task_runner_ = base::ThreadTaskRunnerHandle::Get();
    quit_closure_ = base::BindOnce(
        [](scoped_refptr<base::TaskRunner> testcast_task_runner,
           base::RunLoop* run_loop) {
          ASSERT_TRUE(testcast_task_runner->RunsTasksInCurrentSequence());
          run_loop->Quit();
        },
        testcase_task_runner_, &wait_for_quit_closure_run_loop_);
  }

  void TearDown() override { wait_for_quit_closure_run_loop_.Run(); }

 protected:
  enum class TestMethodExpectation {
    kMustBeExecutedOnTaskRunner,
    kMustNotBeExecutedOnTaskRunner
  };

  void RunQuitClosureGetsInvokedTestCase(EmbeddedServiceInfo info) {
    EXPECT_CALL(mock_service_factory_, Run()).WillOnce(Invoke([]() {
      return std::make_unique<MockService>();
    }));
    info.factory = mock_service_factory_.Get();

    scoped_refptr<EmbeddedInstanceManager> instance_manager =
        new EmbeddedInstanceManager(kArbitraryServiceName, info,
                                    std::move(quit_closure_));

    mojom::ServicePtr service_ptr;
    instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

    // Exercise: Release the instance after a call to Shutdown() but before
    // quit_closure has been called. We expect that |quit_closure| will
    // eventually get invoked.
    instance_manager->ShutDown();
    instance_manager = nullptr;
  }

  void RunServiceMethodGetsInvokedTestCase(
      EmbeddedServiceInfo info,
      scoped_refptr<base::SingleThreadTaskRunner> expectation_task_runner,
      TestMethodExpectation expectation) {
    base::RunLoop wait_loop;

    // When the mock service gets created, set an expectation that an arbitrary
    // method of the service, which we call in Exercise 1 below, will get
    // executed on a thread different from the current one. We (arbitrarily)
    // choose method OnStart() for this purpose.
    EXPECT_CALL(mock_service_factory_, Run())
        .WillOnce(Invoke([this, expectation_task_runner, &wait_loop,
                          expectation]() {
          auto mock_service = std::make_unique<MockService>();
          EXPECT_CALL(*mock_service, OnStart())
              .WillOnce(Invoke([this, &wait_loop, expectation_task_runner,
                                expectation]() {
                switch (expectation) {
                  case TestMethodExpectation::kMustBeExecutedOnTaskRunner:
                    ASSERT_TRUE(
                        expectation_task_runner->BelongsToCurrentThread());
                    break;
                  case TestMethodExpectation::kMustNotBeExecutedOnTaskRunner:
                    ASSERT_FALSE(
                        expectation_task_runner->BelongsToCurrentThread());
                    break;
                }
                testcase_task_runner_->PostTask(FROM_HERE,
                                                wait_loop.QuitClosure());
              }));
          return mock_service;
        }));
    info.factory = mock_service_factory_.Get();

    scoped_refptr<EmbeddedInstanceManager> instance_manager =
        new EmbeddedInstanceManager(kArbitraryServiceName, info,
                                    std::move(quit_closure_));
    mojom::ServicePtr service_ptr;
    instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

    // Exercise: Call the method chosen in the expectation setup in order to
    // verify that it gets executed on the expected task runner.
    Identity arbitrary_identity;
    ASSERT_TRUE(service_ptr.is_bound());
    service_ptr->OnStart(
        arbitrary_identity,
        base::BindOnce([](mojom::ConnectorRequest,
                          mojom::ServiceControlAssociatedRequest) {}));

    wait_loop.Run();

    // We must cleanly shut down |instance_manager|, because otherwise it will
    // not release the MockService instance and the test runner will complain.
    instance_manager->ShutDown();
  }

  void RunQuitClosureGetsInvokedWhenServiceQuitsTestCase(
      EmbeddedServiceInfo info) {
    // We set up a mock service to invoke context()->QuitNow()
    // synchronously as a reaction to an invocation of (arbitrarily chosen)
    // method OnStart(). This ensures that the call is made from the correct
    // sequence, i.e. the sequence the service is operated on.
    EXPECT_CALL(mock_service_factory_, Run()).WillOnce(Invoke([]() {
      auto mock_service = std::make_unique<MockService>();
      auto* mock_service_ptr = mock_service.get();
      EXPECT_CALL(*mock_service, OnStart())
          .WillOnce(Invoke([mock_service_ptr]() {
            mock_service_ptr->InvokeContexQuitNow();
          }));
      return mock_service;
    }));
    info.factory = mock_service_factory_.Get();

    scoped_refptr<EmbeddedInstanceManager> instance_manager =
        new EmbeddedInstanceManager(kArbitraryServiceName, info,
                                    std::move(quit_closure_));

    mojom::ServicePtr service_ptr;
    instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

    // Exercise: Call the method chosen in the expectation setup in order to
    // trigger the call to QuitNow().
    Identity arbitrary_identity;
    ASSERT_TRUE(service_ptr.is_bound());
    service_ptr->OnStart(
        arbitrary_identity,
        base::BindOnce([](mojom::ConnectorRequest,
                          mojom::ServiceControlAssociatedRequest) {}));

    // During TearDown, we expect that |quit_closure_| gets invoked even though
    // we never called |instance_manager->ShutDown()|.
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::MockCallback<EmbeddedServiceInfo::ServiceFactory> mock_service_factory_;
  base::RunLoop wait_for_quit_closure_run_loop_;
  base::OnceClosure quit_closure_;
  scoped_refptr<base::TaskRunner> testcase_task_runner_;
};

TEST_F(EmbeddedInstanceManagerTest,
       QuitClosureGetsInvokedWhenUsingDefaultRunner) {
  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  RunQuitClosureGetsInvokedTestCase(info);
}

TEST_F(EmbeddedInstanceManagerTest,
       QuitClosureGetsInvokedWhenUsingDedicatedThread) {
  EmbeddedServiceInfo info;
  info.use_own_thread = true;
  RunQuitClosureGetsInvokedTestCase(info);
}

TEST_F(
    EmbeddedInstanceManagerTest,
    QuitClosureGetsInvokedWhenUsingExplicitlyGivenTaskRunnerDifferentFromOwner) {
  base::Thread service_thread(kArbitraryThreadName);
  service_thread.Start();
  auto service_thread_task_runner = service_thread.task_runner();

  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  info.task_runner = service_thread_task_runner;

  RunQuitClosureGetsInvokedTestCase(info);
}

TEST_F(
    EmbeddedInstanceManagerTest,
    QuitClosureGetsInvokedWhenUsingExplicitlyGivenTaskRunnerBeingSameAsOwner) {
  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  info.task_runner = base::ThreadTaskRunnerHandle::Get();
  RunQuitClosureGetsInvokedTestCase(info);
}

TEST_F(EmbeddedInstanceManagerTest,
       ServiceMethodGetsInvokedOnDefaultTaskRunner) {
  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  RunServiceMethodGetsInvokedTestCase(
      info, base::ThreadTaskRunnerHandle::Get(),
      TestMethodExpectation::kMustBeExecutedOnTaskRunner);
}

TEST_F(EmbeddedInstanceManagerTest, ServiceMethodGetsInvokedOnDedicatedThread) {
  EmbeddedServiceInfo info;
  info.use_own_thread = true;
  RunServiceMethodGetsInvokedTestCase(
      info, base::ThreadTaskRunnerHandle::Get(),
      TestMethodExpectation::kMustNotBeExecutedOnTaskRunner);
}

TEST_F(EmbeddedInstanceManagerTest,
       ServiceMethodGetsInvokedOnExplicitlyGivenTaskRunnerDifferentFromOwner) {
  base::Thread service_thread(kArbitraryThreadName);
  service_thread.Start();
  auto service_thread_task_runner = service_thread.task_runner();

  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  info.task_runner = service_thread_task_runner;
  RunServiceMethodGetsInvokedTestCase(
      info, service_thread_task_runner,
      TestMethodExpectation::kMustBeExecutedOnTaskRunner);
}

TEST_F(EmbeddedInstanceManagerTest,
       ServiceMethodGetsInvokedOnExplicitlyGivenTaskRunnerBeingSameAsOwner) {
  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  info.task_runner = base::ThreadTaskRunnerHandle::Get();
  RunServiceMethodGetsInvokedTestCase(
      info, base::ThreadTaskRunnerHandle::Get(),
      TestMethodExpectation::kMustBeExecutedOnTaskRunner);
}

TEST_F(EmbeddedInstanceManagerTest,
       QuitClosureGetsInvokedWhenServiceQuits_DefaultTaskRunner) {
  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  RunQuitClosureGetsInvokedWhenServiceQuitsTestCase(info);
}

TEST_F(EmbeddedInstanceManagerTest,
       QuitClosureGetsInvokedWhenServiceQuits_DedicatedThread) {
  EmbeddedServiceInfo info;
  info.use_own_thread = true;
  RunQuitClosureGetsInvokedWhenServiceQuitsTestCase(info);
}

TEST_F(
    EmbeddedInstanceManagerTest,
    QuitClosureGetsInvokedWhenServiceQuits_ExplicitlyGivenTaskRunnerDifferentFromOwner) {
  base::Thread service_thread(kArbitraryThreadName);
  service_thread.Start();
  auto service_thread_task_runner = service_thread.task_runner();

  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  info.task_runner = service_thread_task_runner;

  RunQuitClosureGetsInvokedWhenServiceQuitsTestCase(info);
}

TEST_F(
    EmbeddedInstanceManagerTest,
    QuitClosureGetsInvokedWhenServiceQuits_ExplicitlyGivenTaskRunnerBeingSameAsOwner) {
  EmbeddedServiceInfo info;
  info.use_own_thread = false;
  info.task_runner = base::ThreadTaskRunnerHandle::Get();
  RunQuitClosureGetsInvokedWhenServiceQuitsTestCase(info);
}

}  // namespace service_manager
