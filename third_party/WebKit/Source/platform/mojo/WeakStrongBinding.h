// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WeakStrongBinding_h
#define WeakStrongBinding_h

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/connection_error_callback.h"
#include "mojo/public/cpp/bindings/filter_chain.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/message_header_validator.h"
#include "mojo/public/cpp/system/core.h"
#include "platform/mojo/WeakBinding.h"

namespace blink {

class InterfaceInvalidator;

template <typename Interface>
class WeakStrongBinding;

template <typename Interface>
using WeakStrongBindingPtr = base::WeakPtr<WeakStrongBinding<Interface>>;

// This is a wrapper around a StrongBinding that binds it to an interface
// invalidator. When the invalidator is destroyed or a connection error is
// detected, the interface implementation is deleted. If the task runner that a
// WeakStrongBinding is bound on is stopped, the connection error handler will
// not be invoked and the implementation will not be deleted.
//
// To use, call WeakStrongBinding<T>::Create() (see below) or the helper
// MakeWeakStrongBinding function:
//
//   MakeWeakStrongBinding(std::make_unique<FooImpl>(),
//                         std::move(foo_request));
//
template <typename Interface>
class WeakStrongBinding {
 public:
  // Create a new WeakStrongBinding instance. The instance owns itself, cleaning
  // up only in the event of a pipe connection error or invalidation. Returns a
  // WeakPtr to the new WeakStrongBinding instance.
  static WeakStrongBindingPtr<Interface> Create(
      std::unique_ptr<Interface> impl,
      mojo::InterfaceRequest<Interface> request,
      InterfaceInvalidator* invalidator) {
    WeakStrongBinding* binding =
        new WeakStrongBinding(std::move(impl), std::move(request), invalidator);
    return binding->weak_factory_.GetWeakPtr();
  }

  // Note: The error handler must not delete the interface implementation.
  //
  // This method may only be called after this WeakStrongBinding has been bound
  // to a message pipe.
  void set_connection_error_handler(base::OnceClosure error_handler) {
    DCHECK(binding_.is_bound());
    connection_error_handler_ = std::move(error_handler);
  }

  // Stops processing incoming messages until
  // ResumeIncomingMethodCallProcessing().
  // Outgoing messages are still sent.
  //
  // No errors are detected on the message pipe while paused.
  //
  // This method may only be called if the object has been bound to a message
  // pipe and there are no associated interfaces running.
  void PauseIncomingMethodCallProcessing() {
    binding_.PauseIncomingMethodCallProcessing();
  }
  void ResumeIncomingMethodCallProcessing() {
    binding_.ResumeIncomingMethodCallProcessing();
  }

  // Forces the binding to close. This destroys the WeakStrongBinding instance.
  void Close() { delete this; }

  Interface* impl() { return impl_.get(); }

  // Sends a message on the underlying message pipe and runs the current
  // message loop until its response is received. This can be used in tests to
  // verify that no message was sent on a message pipe in response to some
  // stimulus.
  void FlushForTesting() { binding_.FlushForTesting(); }

 private:
  WeakStrongBinding(std::unique_ptr<Interface> impl,
                    mojo::InterfaceRequest<Interface> request,
                    InterfaceInvalidator* invalidator)
      : impl_(std::move(impl)),
        binding_(impl_.get(), std::move(request), invalidator),
        weak_factory_(this) {
    binding_.set_connection_error_handler(base::BindRepeating(
        &WeakStrongBinding::OnConnectionError, base::Unretained(this)));
  }

  ~WeakStrongBinding() {}

  void OnConnectionError() {
    if (connection_error_handler_) {
      std::move(connection_error_handler_).Run();
    }
    Close();
  }

  std::unique_ptr<Interface> impl_;
  base::OnceClosure connection_error_handler_;
  WeakBinding<Interface> binding_;
  base::WeakPtrFactory<WeakStrongBinding> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WeakStrongBinding);
};

template <typename Interface, typename Impl>
WeakStrongBindingPtr<Interface> MakeWeakStrongBinding(
    std::unique_ptr<Impl> impl,
    mojo::InterfaceRequest<Interface> request,
    InterfaceInvalidator* invalidator) {
  return WeakStrongBinding<Interface>::Create(std::move(impl),
                                              std::move(request), invalidator);
}

}  // namespace blink

#endif  // WeakStrongBinding_h
