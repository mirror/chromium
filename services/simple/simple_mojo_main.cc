// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/simple/public/interfaces/simple.mojom.h"

// This is a small command-line executable that uses the Mojo-based
// simple.mojom.Math interface to add two 32-bit integers. Its purpose is
// to check that Mojo works as expected, as well as measure the total code
// size added by linking in Mojo and its dependencies into the binary.
//
// No performance measurement is being performed here.
//
// TODO(digit): Might add some way to measure RAM usage just before
//              exiting the program.

// Minimal implementation of the Math interface here.
class SimpleMojoMathImpl : public simple::mojom::Math {
 public:
  explicit SimpleMojoMathImpl(simple::mojom::MathRequest request)
      : bindings_(this, std::move(request)) {}

  void Add(int32_t x, int32_t y, AddCallback callback) override {
    int32_t sum = x + y;
    std::move(callback).Run(sum);
  }

 private:
  mojo::Binding<simple::mojom::Math> bindings_;

  DISALLOW_COPY_AND_ASSIGN(SimpleMojoMathImpl);
};

class MathBase {
 public:
  virtual int32_t Add(int32_t, int32_t) = 0;
};

class MathDirect : public MathBase {
 public:
  int32_t Add(int32_t x, int32_t y) override { return x + y; }
};

class MathWithMojo : public MathBase {
 public:
  MathWithMojo() : math_(), impl_(mojo::MakeRequest(&math_)) {}

  int32_t Add(int32_t x, int32_t y) override {
    int32_t result;
    (void)math_->Add(x, y, &result);
    return result;
  }

 private:
  simple::mojom::MathPtr math_;
  SimpleMojoMathImpl impl_;
};

int main() {
  // Initialize Mojo EDK
  mojo::edk::Init();

  // Initialize Mojo IPC support, required for bindings even when implementing
  // in-process calls.
  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  // As long as this object is alive, all EDK API surface relevant to IPC
  // connections is usable and message pipes which span a process boundary will
  // continue to function.
  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  // Create a message loop for the current thread, otherwise Mojo will
  // CHECK() at runtime.
  base::MessageLoop loop;

  // Perform task.
  int x = 10, y = 20;

  MathDirect direct;
  printf("direct: %d + %d = %d\n", x, y, direct.Add(x, y));

  MathWithMojo mojo;
  printf("mojo: %d + %d = %d\n", x, y, mojo.Add(x, y));

  return 0;
}
