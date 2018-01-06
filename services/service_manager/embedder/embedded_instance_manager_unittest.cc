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

enum class ServiceTaskRunnerCase {
  kDefaultTaskRunner,
  kUseDedicatedThread,
  kExplicitlyGivenTaskRunnerDifferentFromOwner,
  kExplicitlyGivenTaskRunnerSameAsOwner
};

static const std::string kArbitraryServiceName = "test_service";
static const std::string kArbitraryThreadName = "test_thread";

}  // namespace

class EmbeddedInstanceManagerTest
    : public testing::TestWithParam<ServiceTaskRunnerCase> {
 public:
  void SetUp() override {
    testcase_task_runner_ = base::ThreadTaskRunnerHandle::Get();
    quit_closure_ =
        base::BindRepeating(&EmbeddedInstanceManagerTest::OnQuitClosureInvoked,
                            base::Unretained(this));

    switch (GetParam()) {
      case ServiceTaskRunnerCase::kDefaultTaskRunner:
        service_info_.use_own_thread = false;
        break;
      case ServiceTaskRunnerCase::kUseDedicatedThread:
        service_info_.use_own_thread = true;
        break;
      case ServiceTaskRunnerCase::kExplicitlyGivenTaskRunnerDifferentFromOwner:
        service_thread_ = std::make_unique<base::Thread>(kArbitraryThreadName);
        service_thread_->Start();
        service_info_.use_own_thread = false;
        service_info_.task_runner = service_thread_->task_runner();
        break;
      case ServiceTaskRunnerCase::kExplicitlyGivenTaskRunnerSameAsOwner:
        service_info_.use_own_thread = false;
        service_info_.task_runner = base::ThreadTaskRunnerHandle::Get();
        break;
    }
  }

 protected:
  enum class TestMethodExpectation {
    kMustBeExecutedOnTaskRunner,
    kMustNotBeExecutedOnTaskRunner
  };

  void RunServiceMethodGetsInvokedTestCase(
      scoped_refptr<base::TaskRunner> expectation_task_runner,
      TestMethodExpectation expectation) {
    base::RunLoop wait_loop;

    // When the mock service gets created, set an expectation that an arbitrary
    // method of the service, which we call in the exercise below, will get
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
                        expectation_task_runner->RunsTasksInCurrentSequence());
                    break;
                  case TestMethodExpectation::kMustNotBeExecutedOnTaskRunner:
                    ASSERT_FALSE(
                        expectation_task_runner->RunsTasksInCurrentSequence());
                    break;
                }
                testcase_task_runner_->PostTask(FROM_HERE,
                                                wait_loop.QuitClosure());
              }));
          return mock_service;
        }));
    service_info_.factory = mock_service_factory_.Get();

    scoped_refptr<EmbeddedInstanceManager> instance_manager =
        new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                    quit_closure_);
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
    PrepareWaitingForQuitClosureInvocation();
    instance_manager->ShutDown();
    WaitForQuitClosureInvocation();
  }

  // Must be called at a time before |quit_closure_| is invoked.
  void PrepareWaitingForQuitClosureInvocation() {
    if (wait_for_quit_closure_run_loop_)
      CHECK(false) << "Already prepared to wait for quit closure invocation";

    wait_for_quit_closure_run_loop_ = std::make_unique<base::RunLoop>();
  }

  // Must be preceded by a call to PrepareWaitingForQuitClosureInvocation().
  // Can be called either before or after |quit_closure_| has been invoked.
  void WaitForQuitClosureInvocation() {
    if (!wait_for_quit_closure_run_loop_) {
      CHECK(false)
          << "Must call PrepareWaitingForQuitClosureInvocation() first";
    }
    wait_for_quit_closure_run_loop_->Run();
    wait_for_quit_closure_run_loop_.reset();
  }

  void OnQuitClosureInvoked() {
    ASSERT_TRUE(testcase_task_runner_->RunsTasksInCurrentSequence());
    if (!wait_for_quit_closure_run_loop_)
      return;
    wait_for_quit_closure_run_loop_->Quit();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  EmbeddedServiceInfo service_info_;
  base::MockCallback<EmbeddedServiceInfo::ServiceFactory> mock_service_factory_;
  std::unique_ptr<base::RunLoop> wait_for_quit_closure_run_loop_;
  base::RepeatingClosure quit_closure_;
  scoped_refptr<base::TaskRunner> testcase_task_runner_;
  std::unique_ptr<base::Thread> service_thread_;
};

TEST_P(EmbeddedInstanceManagerTest,
       ReleaseInstanceAfterShutdownWithoutWaitingForQuitClosure) {
  EXPECT_CALL(mock_service_factory_, Run()).WillOnce(Invoke([]() {
    return std::make_unique<MockService>();
  }));
  service_info_.factory = mock_service_factory_.Get();

  scoped_refptr<EmbeddedInstanceManager> instance_manager =
      new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                  quit_closure_);

  mojom::ServicePtr service_ptr;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

  PrepareWaitingForQuitClosureInvocation();

  // Exercise: Release the instance after a call to Shutdown() but before
  // quit_closure has been called. We expect that |quit_closure| will
  // eventually get invoked.
  instance_manager->ShutDown();
  instance_manager = nullptr;

  WaitForQuitClosureInvocation();
}

// Tests that a method exposed by an example service gets invoked on a sequence
// matching what has been specified via the EmbeddedServiceInfo passed to the
// constructor.
TEST_P(EmbeddedInstanceManagerTest, ServiceMethodGetsInvoked) {
  scoped_refptr<base::TaskRunner> expectation_task_runner;
  TestMethodExpectation expectation_case =
      TestMethodExpectation::kMustBeExecutedOnTaskRunner;
  switch (GetParam()) {
    case ServiceTaskRunnerCase::kDefaultTaskRunner:
      expectation_task_runner = base::ThreadTaskRunnerHandle::Get();
      expectation_case = TestMethodExpectation::kMustBeExecutedOnTaskRunner;
      break;
    case ServiceTaskRunnerCase::kUseDedicatedThread:
      expectation_task_runner = base::ThreadTaskRunnerHandle::Get();
      expectation_case = TestMethodExpectation::kMustNotBeExecutedOnTaskRunner;
      break;
    case ServiceTaskRunnerCase::kExplicitlyGivenTaskRunnerDifferentFromOwner:
      expectation_task_runner = service_thread_->task_runner();
      expectation_case = TestMethodExpectation::kMustBeExecutedOnTaskRunner;
      break;
    case ServiceTaskRunnerCase::kExplicitlyGivenTaskRunnerSameAsOwner:
      expectation_task_runner = base::ThreadTaskRunnerHandle::Get();
      expectation_case = TestMethodExpectation::kMustBeExecutedOnTaskRunner;
      break;
  }
  RunServiceMethodGetsInvokedTestCase(std::move(expectation_task_runner),
                                      expectation_case);
}

TEST_P(EmbeddedInstanceManagerTest, QuitClosureGetsInvokedWhenServiceQuits) {
  // We set up a mock service to invoke context()->QuitNow()
  // synchronously as a reaction to an invocation of (arbitrarily chosen)
  // method OnStart(). This ensures that the call is made from the correct
  // sequence, i.e. the sequence the service is operated on.
  EXPECT_CALL(mock_service_factory_, Run()).WillOnce(Invoke([]() {
    auto mock_service = std::make_unique<MockService>();
    auto* mock_service_ptr = mock_service.get();
    EXPECT_CALL(*mock_service, OnStart()).WillOnce(Invoke([mock_service_ptr]() {
      mock_service_ptr->InvokeContexQuitNow();
    }));
    return mock_service;
  }));
  service_info_.factory = mock_service_factory_.Get();

  scoped_refptr<EmbeddedInstanceManager> instance_manager =
      new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                  quit_closure_);

  mojom::ServicePtr service_ptr;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

  PrepareWaitingForQuitClosureInvocation();

  // Exercise: Call the method chosen in the expectation setup in order to
  // trigger the call to QuitNow().
  Identity arbitrary_identity;
  ASSERT_TRUE(service_ptr.is_bound());
  service_ptr->OnStart(
      arbitrary_identity,
      base::BindOnce([](mojom::ConnectorRequest,
                        mojom::ServiceControlAssociatedRequest) {}));

  // We expect that |quit_closure_| gets invoked even though we never called
  //|instance_manager->ShutDown()|.
  WaitForQuitClosureInvocation();
}

// Tests that calling ShutDown() without having called BindServiceRequest()
// does not lead to an invocation of |quit_closure|.
TEST_P(EmbeddedInstanceManagerTest,
       ShutdownWithoutHavingBoundAnyServiceRequest) {
  EXPECT_CALL(mock_service_factory_, Run()).Times(0);
  service_info_.factory = mock_service_factory_.Get();

  base::MockCallback<base::RepeatingClosure> mock_quit_closure;
  EXPECT_CALL(mock_quit_closure, Run()).Times(0);

  scoped_refptr<EmbeddedInstanceManager> instance_manager =
      new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                  mock_quit_closure.Get());
  base::RunLoop wait_loop;
  instance_manager->SetEventHandlingCompleteObserver(base::BindRepeating(
      [](base::RunLoop* wait_loop) { wait_loop->Quit(); }, &wait_loop));

  instance_manager->ShutDown();
  wait_loop.Run();
}

// Tests that |quit_closure| get invoked only once in total when ShutDown() is
// called after the service has already quit.
TEST_P(EmbeddedInstanceManagerTest,
       ShutdownIsCalledAfterServiceHasQuit_DefaultTaskRunner) {
  // We set up a mock service to invoke context()->QuitNow()
  // synchronously as a reaction to an invocation of (arbitrarily chosen)
  // method OnStart(). This ensures that the call is made from the correct
  // sequence, i.e. the sequence the service is operated on.
  EXPECT_CALL(mock_service_factory_, Run()).WillOnce(Invoke([]() {
    auto mock_service = std::make_unique<MockService>();
    auto* mock_service_ptr = mock_service.get();
    EXPECT_CALL(*mock_service, OnStart()).WillOnce(Invoke([mock_service_ptr]() {
      mock_service_ptr->InvokeContexQuitNow();
    }));
    return mock_service;
  }));
  service_info_.factory = mock_service_factory_.Get();

  base::MockCallback<base::RepeatingClosure> mock_quit_closure;
  EXPECT_CALL(mock_quit_closure, Run()).Times(1);

  scoped_refptr<EmbeddedInstanceManager> instance_manager =
      new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                  mock_quit_closure.Get());

  base::MockCallback<base::RepeatingClosure> event_handling_complete_observer;
  instance_manager->SetEventHandlingCompleteObserver(
      event_handling_complete_observer.Get());

  mojom::ServicePtr service_ptr;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

  base::RunLoop wait_for_service_quit;
  EXPECT_CALL(event_handling_complete_observer, Run())
      .WillOnce(
          Invoke([&wait_for_service_quit]() { wait_for_service_quit.Quit(); }));

  // Exercise part 1:
  // Call the method chosen in the expectation setup in order to
  // trigger the call to QuitNow().
  Identity arbitrary_identity;
  ASSERT_TRUE(service_ptr.is_bound());
  service_ptr->OnStart(
      arbitrary_identity,
      base::BindOnce([](mojom::ConnectorRequest,
                        mojom::ServiceControlAssociatedRequest) {}));
  wait_for_service_quit.Run();

  base::RunLoop wait_for_shutdown;
  EXPECT_CALL(event_handling_complete_observer, Run())
      .WillOnce(Invoke([&wait_for_shutdown]() { wait_for_shutdown.Quit(); }));

  // Exercise part 2:
  instance_manager->ShutDown();
  wait_for_shutdown.Run();
}

TEST_P(EmbeddedInstanceManagerTest, BindServiceRequestAfterServiceHasQuit) {
  // We set up a mock service to invoke context()->QuitNow()
  // synchronously as a reaction to an invocation of (arbitrarily chosen)
  // method OnStart(). This ensures that the call is made from the correct
  // sequence, i.e. the sequence the service is operated on.
  EXPECT_CALL(mock_service_factory_, Run()).WillOnce(Invoke([]() {
    auto mock_service = std::make_unique<MockService>();
    auto* mock_service_ptr = mock_service.get();
    EXPECT_CALL(*mock_service, OnStart()).WillOnce(Invoke([mock_service_ptr]() {
      mock_service_ptr->InvokeContexQuitNow();
    }));
    return mock_service;
  }));
  service_info_.factory = mock_service_factory_.Get();

  scoped_refptr<EmbeddedInstanceManager> instance_manager =
      new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                  quit_closure_);

  base::MockCallback<base::RepeatingClosure> event_handling_complete_observer;
  instance_manager->SetEventHandlingCompleteObserver(
      event_handling_complete_observer.Get());

  mojom::ServicePtr service_ptr;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));

  base::RunLoop wait_for_service_quit;
  EXPECT_CALL(event_handling_complete_observer, Run())
      .WillOnce(
          Invoke([&wait_for_service_quit]() { wait_for_service_quit.Quit(); }));

  // Exercise part 1:
  // Call the method chosen in the expectation setup in order to
  // trigger the call to QuitNow().
  Identity arbitrary_identity;
  ASSERT_TRUE(service_ptr.is_bound());
  service_ptr->OnStart(
      arbitrary_identity,
      base::BindOnce([](mojom::ConnectorRequest,
                        mojom::ServiceControlAssociatedRequest) {}));
  wait_for_service_quit.Run();
  testing::Mock::VerifyAndClearExpectations(&event_handling_complete_observer);

  // Exercise part 2:
  // Bind a new service request and verify that a service method can be
  // executed.
  base::RunLoop wait_for_service_method_execution;
  EXPECT_CALL(mock_service_factory_, Run())
      .WillOnce(Invoke([&wait_for_service_method_execution]() {
        auto mock_service = std::make_unique<MockService>();
        EXPECT_CALL(*mock_service, OnStart())
            .WillOnce(Invoke([&wait_for_service_method_execution]() {
              wait_for_service_method_execution.Quit();
            }));
        return mock_service;
      }));
  mojom::ServicePtr service_ptr_2;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr_2));
  ASSERT_TRUE(service_ptr_2.is_bound());
  service_ptr_2->OnStart(
      arbitrary_identity,
      base::BindOnce([](mojom::ConnectorRequest,
                        mojom::ServiceControlAssociatedRequest) {}));
  wait_for_service_method_execution.Run();

  // Cleanly shut down
  PrepareWaitingForQuitClosureInvocation();
  instance_manager->ShutDown();
  WaitForQuitClosureInvocation();
}

// Tests for issues when service quit races against binding a new service
// request.
TEST_P(EmbeddedInstanceManagerTest,
       BindSecondServiceRequestWhileFirstServiceInstanceQuits) {
  base::RunLoop wait_for_service_factory_invocation;

  // We set up a mock service to invoke context()->QuitNow()
  // synchronously as a reaction to an invocation of (arbitrarily chosen)
  // method OnStart(). This ensures that the call is made from the correct
  // sequence, i.e. the sequence the service is operated on.
  EXPECT_CALL(mock_service_factory_, Run())
      .WillOnce(Invoke([&wait_for_service_factory_invocation, this]() {
        auto mock_service = std::make_unique<MockService>();
        auto* mock_service_ptr = mock_service.get();
        EXPECT_CALL(*mock_service, OnStart())
            .WillOnce(Invoke([mock_service_ptr]() {
              mock_service_ptr->InvokeContexQuitNow();
            }));
        testcase_task_runner_->PostTask(
            FROM_HERE, wait_for_service_factory_invocation.QuitClosure());
        return mock_service;
      }));
  service_info_.factory = mock_service_factory_.Get();

  scoped_refptr<EmbeddedInstanceManager> instance_manager =
      new EmbeddedInstanceManager(kArbitraryServiceName, service_info_,
                                  quit_closure_);

  base::MockCallback<base::RepeatingClosure> event_handling_complete_observer;
  instance_manager->SetEventHandlingCompleteObserver(
      event_handling_complete_observer.Get());

  mojom::ServicePtr service_ptr;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr));
  wait_for_service_factory_invocation.Run();

  base::RunLoop wait_for_service_quit;
  EXPECT_CALL(event_handling_complete_observer, Run())
      .WillOnce(
          Invoke([&wait_for_service_quit]() { wait_for_service_quit.Quit(); }));

  // Exercise:
  // Call the method chosen in the expectation setup in order to
  // trigger the call to QuitNow(). Without waiting for it to finish processing,
  // bind a second service request. In which order they get processed is
  // not defined. As a client, we do not need to care. All we care about is
  // that the newly bound service request comes out operational.
  Identity arbitrary_identity;
  ASSERT_TRUE(service_ptr.is_bound());
  service_ptr->OnStart(
      arbitrary_identity,
      base::BindOnce([](mojom::ConnectorRequest,
                        mojom::ServiceControlAssociatedRequest) {}));
  base::RunLoop wait_for_service_method_execution;
  EXPECT_CALL(mock_service_factory_, Run())
      .WillOnce(Invoke([&wait_for_service_method_execution]() {
        auto mock_service = std::make_unique<MockService>();
        EXPECT_CALL(*mock_service, OnStart())
            .WillOnce(Invoke([&wait_for_service_method_execution]() {
              wait_for_service_method_execution.Quit();
            }));
        return mock_service;
      }));
  mojom::ServicePtr service_ptr_2;
  instance_manager->BindServiceRequest(mojo::MakeRequest(&service_ptr_2));

  wait_for_service_quit.Run();
  testing::Mock::VerifyAndClearExpectations(&event_handling_complete_observer);

  // Verify that the newly bound service is operational.
  ASSERT_TRUE(service_ptr_2.is_bound());
  service_ptr_2->OnStart(
      arbitrary_identity,
      base::BindOnce([](mojom::ConnectorRequest,
                        mojom::ServiceControlAssociatedRequest) {}));
  wait_for_service_method_execution.Run();

  // Cleanly shut down
  PrepareWaitingForQuitClosureInvocation();
  instance_manager->ShutDown();
  WaitForQuitClosureInvocation();
}

INSTANTIATE_TEST_CASE_P(
    EmbeddedInstanceManagerTests,
    EmbeddedInstanceManagerTest,
    testing::ValuesIn(
        {ServiceTaskRunnerCase::kDefaultTaskRunner,
         ServiceTaskRunnerCase::kUseDedicatedThread,
         ServiceTaskRunnerCase::kExplicitlyGivenTaskRunnerDifferentFromOwner,
         ServiceTaskRunnerCase::kExplicitlyGivenTaskRunnerSameAsOwner}));

}  // namespace service_manager
