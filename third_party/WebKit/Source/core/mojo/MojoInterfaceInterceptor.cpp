// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/mojo/MojoInterfaceInterceptor.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/MojoInterfaceInterceptorCallback.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "core/mojo/MojoHandle.h"
#include "core/mojo/MojoInterfaceRequestEvent.h"
#include "platform/bindings/ScriptState.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

// static
MojoInterfaceInterceptor* MojoInterfaceInterceptor::Create(
    ScriptState* script_state,
    const String& interface_name) {
  return new MojoInterfaceInterceptor(script_state, interface_name);
}

MojoInterfaceInterceptor::~MojoInterfaceInterceptor() {}

void MojoInterfaceInterceptor::start(ExceptionState& exception_state) {
  if (started_)
    return;

  std::string interface_name(interface_name_.Utf8().data());
  service_manager::InterfaceProvider::TestApi test_api(
      GetFrame()->Client()->GetFrameInterfaceProvider());
  if (test_api.HasBinder(interface_name)) {
    exception_state.ThrowDOMException(
        kInvalidModificationError,
        "Interface " + interface_name_ +
            " is already intercepted by another MojoInterfaceInterceptor.");
    return;
  }

  started_ = true;
  test_api.SetBinderForName(interface_name,
                            ConvertToBaseCallback(WTF::Bind(
                                &MojoInterfaceInterceptor::OnInterfaceRequest,
                                WrapWeakPersistent(this))));
}

void MojoInterfaceInterceptor::stop() {
  if (!started_)
    return;

  started_ = false;
  service_manager::InterfaceProvider::TestApi test_api(
      GetFrame()->Client()->GetFrameInterfaceProvider());
  std::string interface_name(interface_name_.Utf8().data());
  DCHECK(test_api.HasBinder(interface_name));
  test_api.ClearBinder(interface_name);
}

DEFINE_TRACE(MojoInterfaceInterceptor) {
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

const AtomicString& MojoInterfaceInterceptor::InterfaceName() const {
  return EventTargetNames::MojoInterfaceInterceptor;
}

ExecutionContext* MojoInterfaceInterceptor::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

bool MojoInterfaceInterceptor::HasPendingActivity() const {
  return started_;
}

void MojoInterfaceInterceptor::ContextDestroyed(ExecutionContext*) {
  stop();
}

MojoInterfaceInterceptor::MojoInterfaceInterceptor(ScriptState* script_state,
                                                   const String& interface_name)
    : ContextLifecycleObserver(ExecutionContext::From(script_state)),
      interface_name_(interface_name) {}

void MojoInterfaceInterceptor::OnInterfaceRequest(
    mojo::ScopedMessagePipeHandle handle) {
  DispatchEvent(MojoInterfaceRequestEvent::Create(
      MojoHandle::Create(mojo::ScopedHandle::From(std::move(handle)))));
}

}  // namespace blink
