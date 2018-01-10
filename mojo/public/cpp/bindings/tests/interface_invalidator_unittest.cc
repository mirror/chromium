// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_invalidator.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"
#include "mojo/public/cpp/bindings/weak_interface_ptr.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class PingServiceImpl : public test::PingService {
 public:
  PingServiceImpl(InterfaceRequest<test::PingService> request)
      : binding_(this, std::move(request)) {}
  ~PingServiceImpl() override {}

  // test::PingService:
  void Ping(const PingCallback& callback) override {
    if (ping_handler_)
      ping_handler_.Run();
    callback.Run();
  }

  void set_ping_handler(const base::RepeatingClosure& handler) {
    ping_handler_ = handler;
  }

  Binding<test::PingService>* binding() { return &binding_; }

 private:
  base::RepeatingClosure ping_handler_;
  Binding<test::PingService> binding_;

  DISALLOW_COPY_AND_ASSIGN(PingServiceImpl);
};

using InterfaceInvalidatorTest = BindingsTestBase;

class InterfaceInvalidatorObserver : public InterfaceInvalidator::Observer {
 public:
  InterfaceInvalidatorObserver(const base::RepeatingClosure& handler) {
    invalidate_handler_ = handler;
  }
  ~InterfaceInvalidatorObserver() {}

  void OnInvalidate() override { invalidate_handler_.Run(); }

 private:
  base::RepeatingClosure invalidate_handler_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceInvalidatorObserver);
};

TEST_P(InterfaceInvalidatorTest, DestroyNotifiesObservers) {
  int called = 0;
  auto inc_called_cb = base::BindLambdaForTesting([&] { called++; });
  InterfaceInvalidatorObserver observer1(inc_called_cb);
  InterfaceInvalidatorObserver observer2(inc_called_cb);
  {
    InterfaceInvalidator invalidator;
    invalidator.AddObserver(&observer1);
    invalidator.AddObserver(&observer2);
    EXPECT_EQ(called, 0);
  }
  EXPECT_EQ(called, 2);
}

void DoSetFlag(bool* flag) {
  *flag = true;
}

TEST_P(InterfaceInvalidatorTest, DestroyInvalidatesWeakInterfacePtr) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  base::RunLoop run_loop;
  bool ping_called = false;
  wptr->Ping(base::BindRepeating(DoSetFlag, &ping_called));
  run_loop.RunUntilIdle();
  ASSERT_TRUE(ping_called);

  bool error_handler_called = false;
  wptr.set_connection_error_handler(
      base::BindRepeating(DoSetFlag, &error_handler_called));

  base::RunLoop run_loop2;
  invalidator.reset();
  impl.set_ping_handler(base::BindRepeating([] { FAIL(); }));
  wptr->Ping(base::BindRepeating([] { FAIL(); }));
  run_loop2.RunUntilIdle();

  EXPECT_TRUE(error_handler_called);
  EXPECT_TRUE(wptr.encountered_error());
  EXPECT_TRUE(wptr);
}

TEST_P(InterfaceInvalidatorTest, InvalidateAfterMessageSent) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  base::RunLoop run_loop;
  bool called = false;
  impl.set_ping_handler(base::BindRepeating(DoSetFlag, &called));
  // The passed in callback will not be called as the interface is invalidated
  // before a response can come back.
  wptr->Ping(base::BindRepeating([] { FAIL(); }));
  invalidator.reset();
  run_loop.RunUntilIdle();
  EXPECT_TRUE(called);
}

TEST_P(InterfaceInvalidatorTest, PassInterfaceThenInvalidate) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  bool impl_called = false;
  impl.set_ping_handler(base::BindRepeating(DoSetFlag, &impl_called));
  wptr.set_connection_error_handler(base::BindRepeating([] { FAIL(); }));

  base::RunLoop run_loop;
  InterfacePtr<test::PingService> ptr(wptr.PassInterface());
  invalidator.reset();
  bool ping_called = false;
  ptr->Ping(base::BindRepeating(DoSetFlag, &ping_called));
  run_loop.RunUntilIdle();

  EXPECT_TRUE(ping_called);
  EXPECT_TRUE(impl_called);
}

TEST_P(InterfaceInvalidatorTest, PassInterfaceOfInvalidatedPtr) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  impl.set_ping_handler(base::BindRepeating([] { FAIL(); }));
  bool error_handler_called = false;
  wptr.set_connection_error_handler(
      base::BindRepeating(DoSetFlag, &error_handler_called));

  base::RunLoop run_loop;
  // This also destroys the original invalidator.
  invalidator = std::make_unique<InterfaceInvalidator>();
  run_loop.RunUntilIdle();
  ASSERT_TRUE(error_handler_called);

  base::RunLoop run_loop2;
  WeakInterfacePtr<test::PingService> wptr2(wptr.PassInterface(),
                                            invalidator.get());
  wptr2->Ping(base::BindRepeating([] { FAIL(); }));
  run_loop2.RunUntilIdle();
}

TEST_P(InterfaceInvalidatorTest, PassInterfaceBeforeFullyInvalidated) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  impl.set_ping_handler(base::BindRepeating([] { FAIL(); }));
  wptr.set_connection_error_handler(base::BindRepeating([] { FAIL(); }));

  base::RunLoop run_loop;
  // This also destroys the original invalidator.
  invalidator = std::make_unique<InterfaceInvalidator>();
  WeakInterfacePtr<test::PingService> wptr2(wptr.PassInterface(),
                                            invalidator.get());
  wptr2->Ping(base::BindRepeating([] { FAIL(); }));
  run_loop.RunUntilIdle();
}

TEST_P(InterfaceInvalidatorTest, InvalidateAfterReset) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));
  wptr.set_connection_error_handler(base::BindRepeating([] { FAIL(); }));

  base::RunLoop run_loop;
  wptr.reset();
  invalidator.reset();
  run_loop.RunUntilIdle();

  EXPECT_FALSE(wptr);
}

TEST_P(InterfaceInvalidatorTest, ResetInvalidatedWeakInterfacePtr) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));
  wptr.set_connection_error_handler(base::BindRepeating([] { FAIL(); }));

  base::RunLoop run_loop;
  invalidator.reset();
  wptr.reset();
  run_loop.RunUntilIdle();
}

TEST_P(InterfaceInvalidatorTest, InvalidateErroredPtr) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  int called = 0;
  wptr.set_connection_error_handler(
      base::BindLambdaForTesting([&] { called++; }));

  base::RunLoop run_loop;
  impl.binding()->Close();
  invalidator.reset();
  run_loop.RunUntilIdle();
  EXPECT_EQ(called, 1);
}

// InterfacePtrs do not set up a proxy until they are used for the first
// time.
TEST_P(InterfaceInvalidatorTest, InvalidateBeforeProxyConfigured) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  base::RunLoop run_loop;
  invalidator.reset();
  wptr->Ping(base::BindRepeating([] { FAIL(); }));
  run_loop.RunUntilIdle();
}

TEST_P(InterfaceInvalidatorTest, MoveChangesInvalidatorObserver) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  auto wptr2(std::move(wptr));
  base::RunLoop run_loop;
  invalidator.reset();
  wptr2->Ping(base::BindRepeating([] { FAIL(); }));
  run_loop.RunUntilIdle();
}

TEST_P(InterfaceInvalidatorTest, MoveInvalidatedPointer) {
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  PingServiceImpl impl(MakeRequest(&wptr, invalidator.get()));

  invalidator.reset();
  auto wptr2(std::move(wptr));
  base::RunLoop run_loop;
  wptr2->Ping(base::BindRepeating([] { FAIL(); }));
  run_loop.RunUntilIdle();
}

INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(InterfaceInvalidatorTest);

}  // namespace
}  // namespace mojo
