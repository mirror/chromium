// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "simple_api.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/simple/public/interfaces/simple.mojom.h"

#include <memory>

#include <cstdio>

#if defined(DEBUG) && DEBUG > 0
#define DBG(...)                                               \
  fprintf(stderr, "%s:%d:%s: ", __FILE__, __LINE__, __func__), \
      fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...) ((void)0)
#endif

namespace {

// Initialize Mojo for this shared library
class MojoInitializer {
 public:
  MojoInitializer() {
    DBG("Initializing Mojo\n");

    // This is inspired from the code at:
    // https://chromium.googlesource.com/chromium/src/+/master/mojo/edk/embedder
    // Differences are:
    // - Use of std::unique_ptr<> for ipc_thread_ and ipc_support_ in order
    //   to ensure that these objects are only destroyed when this object is
    //   destroyed (i.e. when the library is unloaded, or the program exits,
    //   since only a static global instance exists).
    mojo::edk::Init();

#ifdef USE_MOJO_IPC_THREAD
    ipc_thread_.reset(new base::Thread("ipc!"));
    ipc_thread_->StartWithOptions(
        base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

    // As long as this object is alive, all EDK API surface relevant to IPC
    // connections is usable and message pipes which span a process boundary
    // will continue to function.
    ipc_support_.reset(new mojo::edk::ScopedIPCSupport(
        ipc_thread_->task_runner(),
        mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN));
#endif  // USE_MOJO_IPC_THREAD

    // Create a message loop for the current thread, otherwise Mojo will
    // CHECK() at runtime with a message like:
    //     Error: This caller requires a sequenced context (i.e. the current
    //     task needs to run from a SequencedTaskRunner)
    message_loop_.reset(new base::MessageLoop());

    ok_ = true;
  }

  bool ok() const { return ok_; }

 private:
  bool ok_ = false;
  std::unique_ptr<base::Thread> ipc_thread_;
#ifdef USE_MOJO_IPC_THREAD
  std::unique_ptr<mojo::edk::ScopedIPCSupport> ipc_support_;
#endif
  std::unique_ptr<base::MessageLoop> message_loop_;
};

// This variable's constructor will be called when the library is loaded.
MojoInitializer sMojoInit;

// Implementation of the simple:: classes using in-process
// Mojo message pipe (no services).

// Mojo interface implementation comes first.
class MathImpl : public simple::mojom::Math {
 public:
  explicit MathImpl(simple::mojom::MathRequest request)
      : bindings_(this, std::move(request)) {
    DBG("Constructing\n");
  }

  void Add(int32_t x, int32_t y, AddCallback callback) override {
    DBG("Impl::Add %d + %d ...\n", x, y);
    int32_t sum = x + y;
    std::move(callback).Run(sum);
  }

 private:
  mojo::Binding<simple::mojom::Math> bindings_;

  DISALLOW_COPY_AND_ASSIGN(MathImpl);
};

// Now the actual client-side implementation.
class MyMath : public simple::Math {
 public:
  MyMath() : math_(), impl_(mojo::MakeRequest(&math_)) {
    DBG("Constructing\n");
  }

  int32_t Add(int32_t x, int32_t y) override {
    int32_t sum;
#ifdef USE_MOJO_SYNC_METHOD
    math_->Add(x, y, &sum);
#else
    base::RunLoop loop;
    (void)math_->Add(
        x, y,
        base::BindOnce(&MyMath::StoreResult, base::Unretained(&sum)));
    loop.RunUntilIdle();
#endif
    return sum;
  }

 private:
  static void StoreResult(int32_t* sum, int32_t result) { *sum = result; }

  simple::mojom::MathPtr math_;
  MathImpl impl_;
};

// Do the same for simple::Echo
class EchoImpl : public simple::mojom::Echo {
 public:
  explicit EchoImpl(simple::mojom::EchoRequest request)
      : bindings_(this, std::move(request)) {}

  void Ping(const std::string& str, PingCallback callback) override {
    std::move(callback).Run(str);
  }

 private:
  mojo::Binding<simple::mojom::Echo> bindings_;

  DISALLOW_COPY_AND_ASSIGN(EchoImpl);
};

class MyEcho : public simple::Echo {
 public:
  MyEcho() : echo_(), impl_(mojo::MakeRequest(&echo_)) {}

  std::string Ping(const std::string& str) override {
    std::string result;
#ifdef USE_MOJO_SYNC_METHOD
    (void)echo_->Ping(str, &result);
#else
    // TODO(digit): Is there a more efficient way to wait for the result here?
    //              This looks already 20% faster than using the sync wrapper
    //              on Linux!?!
    base::RunLoop loop;
    echo_->Ping(str,
                base::BindOnce(&MyEcho::StoreResult,
                base::Unretained(&result)));
    loop.RunUntilIdle();
#endif
    return result;
  }

 private:
  static void StoreResult(std::string* out, const std::string& result) {
    *out = result;
  }

  simple::mojom::EchoPtr echo_;
  EchoImpl impl_;
};

}  // namespace

simple::Math* createMath() {
  DBG("Creating Math\n");
  CHECK(sMojoInit.ok());
  return new MyMath();
}

simple::Echo* createEcho() {
  CHECK(sMojoInit.ok());
  return new MyEcho();
}

const char* createAbstract() {
  static const char kAbstract[] =
#ifdef USE_MOJO_SYNC_METHOD
      "Use Mojo synchronous C++ bindings"
#else
      "Use Mojo asynchronous C++ bindings"
#endif
#ifdef USE_LAZY_SERIALIZATION
      " with lazy serialization"
#else
      " with forced serialization"
#endif
#ifdef USE_MOJO_IPC_THREAD
      " with IPC thread"
#endif
      "."
      ;
  return kAbstract;
}
