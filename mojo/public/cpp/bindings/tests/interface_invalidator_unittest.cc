// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
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
  PingServiceImpl() {}
  ~PingServiceImpl() override {}

  // test::PingService:
  void Ping(const PingCallback& callback) override { callback.Run(); }

 private:
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
  auto inc_called_cb = base::BindRepeating([](int* var) { (*var)++; }, &called);
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

TEST_P(InterfaceInvalidatorTest, DestroyInvalidatesWeakInterfacePtr) {
  PingServiceImpl impl;
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  Binding<test::PingService> binding(&impl,
                                     MakeRequest(&wptr, invalidator.get()));

  base::RunLoop run_loop;
  bool called = false;
  wptr->Ping(base::BindRepeating([](bool* var) { *var = true; }, &called));
  run_loop.RunUntilIdle();
  EXPECT_TRUE(called);

  base::RunLoop run_loop2;
  invalidator.reset();
  wptr->Ping(base::BindRepeating([]() { NOTREACHED(); }));
  run_loop2.RunUntilIdle();
}

// InterfacePtrs do not set up a proxy until they are used for the first
// time
TEST_P(InterfaceInvalidatorTest, DestroyBeforeProxyConfigured) {
  PingServiceImpl impl;
  WeakInterfacePtr<test::PingService> wptr;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  Binding<test::PingService> binding(&impl,
                                     MakeRequest(&wptr, invalidator.get()));

  base::RunLoop run_loop;
  invalidator.reset();
  wptr->Ping(base::BindRepeating([]() { NOTREACHED(); }));
  run_loop.RunUntilIdle();
}

TEST_P(InterfaceInvalidatorTest, MoveChangesInvalidatorObserver) {
  PingServiceImpl impl;
  auto invalidator = std::make_unique<InterfaceInvalidator>();
  WeakInterfacePtr<test::PingService> wptr2;
  Binding<test::PingService> binding(&impl,
                                     MakeRequest(&wptr2, invalidator.get()));
  auto wptr(std::move(wptr2));

  base::RunLoop run_loop;
  invalidator.reset();
  wptr->Ping(base::BindRepeating([]() { NOTREACHED(); }));
  run_loop.RunUntilIdle();
}

INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(InterfaceInvalidatorTest);

}  // namespace
}  // namespace mojo
