// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/run_loop.h"
#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/task_scheduler.h"
#include "mojo/edk/embedder/embedder.h"
#include "testing/libfuzzer/fuzzers/fuzz.mojom.h"
#include "mojo/public/cpp/bindings/lib/binding_state.h"

using namespace mojo;
using namespace fuzz::mojom;
using namespace mojo::internal;

/* Copied mojo::internal::BindingState to allow for exposing the router_. */
template <typename Interface, typename ImplRefTraits>
class MyBindingState : public BindingStateBase {
 public:
  using ImplPointerType = typename ImplRefTraits::PointerType;

  explicit MyBindingState(ImplPointerType impl) {
    stub_.set_sink(std::move(impl));
  }

  ~MyBindingState() { Close(); }

  void Bind(ScopedMessagePipeHandle handle,
            scoped_refptr<base::SingleThreadTaskRunner> runner) {
    BindingStateBase::BindInternal(
        std::move(handle), runner, Interface::Name_,
        base::MakeUnique<typename Interface::RequestValidator_>(),
        Interface::PassesAssociatedKinds_, Interface::HasSyncMethods_, &stub_,
        Interface::Version_);
  }

  InterfaceRequest<Interface> Unbind() {
    endpoint_client_.reset();
    InterfaceRequest<Interface> request(router_->PassMessagePipe());
    router_ = nullptr;
    return request;
  }

  Interface* impl() { return ImplRefTraits::GetRawPointer(&stub_.sink()); }

  scoped_refptr<MultiplexRouter> router() { return router_; }

 private:
  typename Interface::template Stub_<ImplRefTraits> stub_;

  DISALLOW_COPY_AND_ASSIGN(MyBindingState);
};

/* Copied mojo::internal::Binding to allow for exposing the internal_state_. */
template <typename Interface,
          typename ImplRefTraits = RawPtrImplRefTraits<Interface>>
class MyBinding {
 public:
  using ImplPointerType = typename ImplRefTraits::PointerType;

  // Constructs an incomplete binding that will use the implementation |impl|.
  // The binding may be completed with a subsequent call to the |Bind| method.
  // Does not take ownership of |impl|, which must outlive the binding.
  explicit MyBinding(ImplPointerType impl) : internal_state_(std::move(impl)) {}

  // Constructs a completed binding of |impl| to the message pipe endpoint in
  // |request|, taking ownership of the endpoint. Does not take ownership of
  // |impl|, which must outlive the binding.
  MyBinding(ImplPointerType impl,
          InterfaceRequest<Interface> request,
          scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr)
      : MyBinding(std::move(impl)) {
    Bind(std::move(request), std::move(runner));
  }

  // Tears down the binding, closing the message pipe and leaving the interface
  // implementation unbound.
  ~MyBinding() {}

  // Completes a binding that was constructed with only an interface
  // implementation by removing the message pipe endpoint from |request| and
  // binding it to the previously specified implementation.
  void Bind(InterfaceRequest<Interface> request,
            scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr) {
    internal_state_.Bind(request.PassMessagePipe(), std::move(runner));
  }

  MyBindingState<Interface, ImplRefTraits> internal_state_;

  DISALLOW_COPY_AND_ASSIGN(MyBinding);
};

/* Dummy implementation of the FuzzInterface. */
class FuzzImpl : public FuzzInterface {
 public:
  explicit FuzzImpl(FuzzInterfaceRequest request) :
    binding_(this, std::move(request)) {}

  void FuzzBasic() override { }

  void FuzzBasicResp(FuzzBasicRespCallback callback) override {
    std::move(callback).Run();
  }

  void FuzzBasicSyncResp(FuzzBasicSyncRespCallback callback) override {
    std::move(callback).Run();
  }

  void FuzzArgs(FuzzStructPtr a, FuzzStructPtr b) override {
  }

  void FuzzArgsResp(FuzzStructPtr a, FuzzStructPtr b, FuzzArgsRespCallback callback) override {
    std::move(callback).Run();
  };

  void FuzzArgsSyncResp(FuzzStructPtr a, FuzzStructPtr b, FuzzArgsSyncRespCallback callback) override {
    std::move(callback).Run();
  };


  ~FuzzImpl() { };

  /* Expose the binding to the fuzz harness. */
  MyBinding<FuzzInterface> binding_;
};

void fuzz_message(const uint8_t* data, size_t size, base::RunLoop *run) {
  FuzzInterfacePtr fuzz;
  auto impl = base::MakeUnique<FuzzImpl>(MakeRequest(&fuzz));
  auto router = impl->binding_.internal_state_.router();
  std::vector<ScopedHandle> handles;

  /* Create a mojo message with the appropriate payload size. */
  mojo::Message message(0, 0, size, 0, &handles);

  /* Set the raw message data. */
  memcpy(message.mutable_data(), data, size);

  /* Run the message through header validation, payload validation, and
   * dispatch to the impl. */
  router->SimulateReceivingMessageForTesting(&message);

  /* Allow the harness function to return now. */
  run->Quit();
}

static bool init() {
  /* Initialize the TaskScheduler and mojo EDK. */
  base::TaskScheduler::CreateAndStartWithDefaultParams("MojoFuzzerProcess");
  mojo::edk::Init();
  return true;
}

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static bool inited = init();
  static mojo::internal::ScopedSuppressValidationErrorLoggingForTests log_suppression;
  static base::MessageLoop message_loop(base::MessageLoop::TYPE_UI);
  ALLOW_UNUSED_LOCAL(inited);

  /* Mojo messages must be sent and processed from TaskRunners. Pass the data
   * along to run in an appropriate context, and wait for it to finish. */
  base::RunLoop run;
  message_loop.task_runner()->PostTask(FROM_HERE, base::BindOnce(&fuzz_message, data, size, &run));
  run.Run();

  return 0;
}
