// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/mojo/test/MojoInterfaceInterceptor.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameClient.h"
#include "core/mojo/MojoHandle.h"
#include "core/mojo/test/MojoInterfaceInterceptorInit.h"
#include "core/mojo/test/MojoInterfaceRequestEvent.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerThread.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/WebTaskRunner.h"
#include "platform/bindings/ScriptState.h"
#include "platform/wtf/text/StringUTF8Adaptor.h"
#include "public/platform/Platform.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

// static
MojoInterfaceInterceptor* MojoInterfaceInterceptor::Create(
    ScriptState* script_state,
    const MojoInterfaceInterceptorInit& init_dict) {
  return new MojoInterfaceInterceptor(script_state, init_dict);
}

MojoInterfaceInterceptor::~MojoInterfaceInterceptor() {}

void MojoInterfaceInterceptor::start(ExceptionState& exception_state) {
  if (started_)
    return;

  std::string interface_name =
      StringUTF8Adaptor(interface_name_).AsStringPiece().as_string();
  if (service_name_.IsNull()) {
    service_manager::InterfaceProvider* interface_provider =
        GetInterfaceProvider();
    if (!interface_provider) {
      exception_state.ThrowDOMException(
          kInvalidStateError, "The interface provider is unavailable.");
      return;
    }

    service_manager::InterfaceProvider::TestApi test_api(interface_provider);
    if (test_api.HasBinderForName(interface_name)) {
      exception_state.ThrowDOMException(
          kInvalidModificationError,
          "Interface " + interface_name_ +
              " is already intercepted by another MojoInterfaceInterceptor.");
      return;
    }

    test_api.SetBinderForName(interface_name,
                              ConvertToBaseCallback(WTF::Bind(
                                  &MojoInterfaceInterceptor::OnInterfaceRequest,
                                  WrapWeakPersistent(this))));
  } else {
    std::string service_name =
        StringUTF8Adaptor(service_name_).AsStringPiece().as_string();
    service_manager::Connector::TestApi test_api(
        Platform::Current()->GetConnector());
    if (test_api.HasBinderOverrideForName(service_name, interface_name)) {
      exception_state.ThrowDOMException(
          kInvalidModificationError,
          "Interface " + interface_name_ + " from service " + service_name_ +
              " is already intercepted by another MojoInterfaceInterceptor.");
      return;
    }

    // When overriding a binder on the Connector interface requests may be made
    // from any thread so this must be a CrossThreadBind.
    test_api.OverrideBinderForTesting(
        service_name, interface_name,
        ConvertToBaseCallback(
            CrossThreadBind(&MojoInterfaceInterceptor::OnInterfaceRequest,
                            WrapCrossThreadWeakPersistent(this))));
  }
  started_ = true;
}

void MojoInterfaceInterceptor::stop() {
  if (!started_)
    return;

  started_ = false;

  std::string interface_name =
      StringUTF8Adaptor(interface_name_).AsStringPiece().as_string();
  if (service_name_.IsNull()) {
    // GetInterfaceProvider() is guaranteed not to return nullptr because this
    // method is called when the context is destroyed.
    service_manager::InterfaceProvider::TestApi test_api(
        GetInterfaceProvider());
    DCHECK(test_api.HasBinderForName(interface_name));
    test_api.ClearBinderForName(interface_name);
  } else {
    std::string service_name =
        StringUTF8Adaptor(service_name_).AsStringPiece().as_string();
    service_manager::Connector::TestApi test_api(
        Platform::Current()->GetConnector());
    DCHECK(test_api.HasBinderOverrideForName(service_name, interface_name));
    test_api.ClearBinderOverrideForName(service_name, interface_name);
  }
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

MojoInterfaceInterceptor::MojoInterfaceInterceptor(
    ScriptState* script_state,
    const MojoInterfaceInterceptorInit& init_dict)
    : ContextLifecycleObserver(ExecutionContext::From(script_state)),
      service_name_(init_dict.serviceName()),
      interface_name_(init_dict.interfaceName()) {}

service_manager::InterfaceProvider*
MojoInterfaceInterceptor::GetInterfaceProvider() const {
  ExecutionContext* context = GetExecutionContext();
  if (!context)
    return nullptr;

  if (context->IsWorkerGlobalScope())
    return &ToWorkerGlobalScope(context)->GetThread()->GetInterfaceProvider();

  LocalFrame* frame = ToDocument(context)->GetFrame();
  if (!frame)
    return nullptr;

  return frame->Client()->GetInterfaceProvider();
}

void MojoInterfaceInterceptor::OnInterfaceRequest(
    mojo::ScopedMessagePipeHandle handle) {
  // A task is posted to dispatch this event because it is possible that:
  //
  // 1. Execution of JavaScript is forbidden in this context as this method is
  //    called synchronously by the InterfaceProvider. Dispatching of the
  //    'interfacerequest' event is therefore scheduled to take place in the
  //    next microtask. This also more closely mirrors the behavior when an
  //    interface request is being satisfied by another process.
  // 2. This request was initiated by the Connector on a separate thread from
  //    the execution context that created object and so a task must be posted
  //    to return to the correct thread.
  TaskRunnerHelper::Get(TaskType::kMicrotask, GetExecutionContext())
      ->PostTask(
          BLINK_FROM_HERE,
          CrossThreadBind(
              &MojoInterfaceInterceptor::DispatchInterfaceRequestEvent,
              WrapCrossThreadPersistent(this), WTF::Passed(std::move(handle))));
}

void MojoInterfaceInterceptor::DispatchInterfaceRequestEvent(
    mojo::ScopedMessagePipeHandle handle) {
  DispatchEvent(MojoInterfaceRequestEvent::Create(
      MojoHandle::Create(mojo::ScopedHandle::From(std::move(handle)))));
}

}  // namespace blink
