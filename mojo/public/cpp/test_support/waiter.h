// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_TEST_SUPPORT_WAITER_H_
#define MOJO_PUBLIC_CPP_TEST_SUPPORT_WAITER_H_

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"

namespace mojo {
namespace test {

namespace internal {

template <typename... Args>
struct WaiterHelper {
  static void CaptureHelper(Args*... out_args,
                            base::OnceClosure continuation,
                            Args... in_args) {
    int ignored[] = {0, Move(std::forward<Args>(in_args), out_args)...};
    ALLOW_UNUSED_LOCAL(ignored);
    std::move(continuation).Run();
  }

 private:
  template <typename T>
  static int Move(T&& source, T* dest) {
    *dest = std::move(source);
    return 0;
  }
};

}  // namespace internal

// Waits for a mojo method callback and captures the results. Allows test code
// to be written in a synchronous style, but does not block the current thread
// like [Sync] mojo methods do.
//
// For mojo methods:
//   DoFoo() => ();
//   DoBar() => (int32 i, bool b);
//
// TEST(FooBarTest) {
//   Waiter waiter;
//   // ...
//   interface_ptr->DoFoo(waiter.Capture());
//   waiter.Wait();
//
//   int i = 0;
//   bool b = false;
//   interface_ptr->DoBar(waiter.Capture(&i, &b));
//   waiter.Wait();
//   EXPECT_EQ(0, i);
//   EXPECT_TRUE(b);
// }
//
// TODO(rockot): This doesn't work for strings or types where the callback
// uses a const&.
class Waiter {
 public:
  Waiter();
  ~Waiter();

  // Stores |args| to be captured later. Can be called more than once.
  // Can be used with an empty argument list.
  template <typename... Args>
  base::OnceCallback<void(Args...)> Capture(Args*... args) {
    DCHECK(!loop_ || !loop_->running());
    loop_ = std::make_unique<base::RunLoop>();
    return base::BindOnce(&internal::WaiterHelper<Args...>::CaptureHelper,
                          args..., loop_->QuitClosure());
  }

  // Runs the run loop to wait for results.
  void Wait();

 private:
  std::unique_ptr<base::RunLoop> loop_;

  DISALLOW_COPY_AND_ASSIGN(Waiter);
};

}  // namespace test
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_TEST_SUPPORT_WAITER_H_
