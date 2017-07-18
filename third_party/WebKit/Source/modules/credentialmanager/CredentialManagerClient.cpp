// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialManagerClient.h"

#include <memory>

#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/LocalFrame.h"
#include "platform/bindings/ScriptState.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Functional.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

namespace {

using RequestCallbacks = CredentialManagerClient::RequestCallbacks;
using NotificationCallbacks = CredentialManagerClient::NotificationCallbacks;

// Takes ownership of NotificationCallbacks
// pointer. When the wrapper is destroyed, if |callbacks| is still alive
// its onError() will get called.
class NotificationCallbacksWrapper {
 public:
  explicit NotificationCallbacksWrapper(NotificationCallbacks*);

  ~NotificationCallbacksWrapper();

  void NotifySuccess();

 private:
  std::unique_ptr<NotificationCallbacks> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(NotificationCallbacksWrapper);
};

NotificationCallbacksWrapper::NotificationCallbacksWrapper(
    NotificationCallbacks* callbacks)
    : callbacks_(callbacks) {}

NotificationCallbacksWrapper::~NotificationCallbacksWrapper() {
  if (callbacks_)
    callbacks_->OnError(kWebCredentialManagerUnknownError);
}

void NotificationCallbacksWrapper::NotifySuccess() {
  // Call onSuccess() and reset callbacks to avoid calling onError() in
  // destructor.
  if (callbacks_) {
    callbacks_->OnSuccess();
    callbacks_.reset();
  }
}

// Takes ownership of RequestCallbacks
// pointer. When the wrapper is destroied, if |callbacks| is still alive
// its onError() will get called.
class RequestCallbacksWrapper {
 public:
  explicit RequestCallbacksWrapper(RequestCallbacks*);

  ~RequestCallbacksWrapper();

  void NotifySuccess(Credential*);
  void NotifyError(WebCredentialManagerError);

 private:
  std::unique_ptr<RequestCallbacks> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(RequestCallbacksWrapper);
};

RequestCallbacksWrapper::RequestCallbacksWrapper(RequestCallbacks* callbacks)
    : callbacks_(callbacks) {}

RequestCallbacksWrapper::~RequestCallbacksWrapper() {
  if (callbacks_)
    callbacks_->OnError(kWebCredentialManagerUnknownError);
}

void RequestCallbacksWrapper::NotifySuccess(Credential* credential) {
  // Call onSuccess() and reset callbacks to avoid calling onError() in
  // destructor.
  if (callbacks_) {
    callbacks_->OnSuccess(credential);
    callbacks_.reset();
  }
}

void RequestCallbacksWrapper::NotifyError(WebCredentialManagerError error) {
  if (callbacks_) {
    callbacks_->OnError(error);
    callbacks_.reset();
  }
}

void RespondToNotificationCallback(
    NotificationCallbacksWrapper* callbacks_wrapper) {
  callbacks_wrapper->NotifySuccess();
}

void RespondToRequestCallback(RequestCallbacksWrapper* callbacks_wrapper,
                              WebCredentialManagerError error,
                              const Persistent<Credential>& credential) {
  if (error == kWebCredentialManagerNoError) {
    callbacks_wrapper->NotifySuccess(credential.Get());
  } else {
    DCHECK(!credential);
    callbacks_wrapper->NotifyError(error);
  }
}

}  // namespace

CredentialManagerClient::CredentialManagerClient(
    ExecutionContext* execution_context) {
  if (!execution_context)
    return;
  LocalFrame* frame = ToDocument(execution_context)->GetFrame();
  DCHECK(frame);
  frame->GetInterfaceProvider().GetInterface(&mojo_cm_service_);
}

CredentialManagerClient::~CredentialManagerClient() {}

DEFINE_TRACE(CredentialManagerClient) {
  Supplement<ExecutionContext>::Trace(visitor);
}

// static
const char* CredentialManagerClient::SupplementName() {
  return "CredentialManagerClient";
}

// static
CredentialManagerClient* CredentialManagerClient::From(
    ExecutionContext* execution_context) {
  if (!execution_context->IsDocument() ||
      !ToDocument(execution_context)->IsInMainFrame()) {
    return 0;
  }

  if (!Supplement<ExecutionContext>::From(execution_context,
                                          SupplementName())) {
    ProvideToExecutionContext(execution_context,
                              new CredentialManagerClient(execution_context));
  }

  return static_cast<CredentialManagerClient*>(
      Supplement<ExecutionContext>::From(execution_context, SupplementName()));
}

// static
void CredentialManagerClient::ProvideToExecutionContext(
    ExecutionContext* execution_context,
    CredentialManagerClient* client) {
  CredentialManagerClient::ProvideTo(
      *execution_context, CredentialManagerClient::SupplementName(), client);
}

void CredentialManagerClient::DispatchFailedSignIn(
    Credential* credential,
    NotificationCallbacks* callbacks) {
  // TODO(engedy): Figure out when this was removed from the spec.
}

void CredentialManagerClient::DispatchStore(Credential* credential,
                                            NotificationCallbacks* callbacks) {
  mojo_cm_service_->Store(
      credential,
      ConvertToBaseCallback(
          WTF::Bind(&RespondToNotificationCallback,
                    WTF::Passed(new NotificationCallbacksWrapper(callbacks)))));
}

void CredentialManagerClient::DispatchPreventSilentAccess(
    NotificationCallbacks* callbacks) {
  mojo_cm_service_->PreventSilentAccess(ConvertToBaseCallback(
      WTF::Bind(&RespondToNotificationCallback,
                WTF::Passed(new NotificationCallbacksWrapper(callbacks)))));
}

void CredentialManagerClient::DispatchGet(
    WebCredentialMediationRequirement mediation,
    bool include_passwords,
    const ::WTF::Vector<KURL>& federations,
    RequestCallbacks* callbacks) {
  mojo_cm_service_->Get(
      mediation, include_passwords, federations,
      ConvertToBaseCallback(
          WTF::Bind(&RespondToRequestCallback,
                    WTF::Passed(new RequestCallbacksWrapper(callbacks)))));
}

}  // namespace blink
