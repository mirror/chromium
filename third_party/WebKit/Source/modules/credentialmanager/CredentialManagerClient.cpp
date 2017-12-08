// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialManagerClient.h"

#include <utility>
#include "common/associated_interfaces/associated_interface_provider.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/page/Page.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/WebCredential.h"
#include "public/platform/WebFederatedCredential.h"
#include "public/platform/WebPasswordCredential.h"

namespace blink {
namespace {

// TODO(vasilii): get rid of the conversions. Everything is to be done by Mojo.
void WebCredentialToCredentialInfo(
    const WebCredential& credential,
    password_manager::mojom::blink::CredentialInfo* out) {
  out->id = credential.Id();
  if (credential.IsPasswordCredential()) {
    out->type = password_manager::mojom::CredentialType::PASSWORD;
    out->password =
        static_cast<const blink::WebPasswordCredential&>(credential).Password();
    out->name =
        static_cast<const blink::WebPasswordCredential&>(credential).Name();
    out->icon =
        static_cast<const blink::WebPasswordCredential&>(credential).IconURL();
  } else {
    DCHECK(credential.IsFederatedCredential());
    out->type = password_manager::mojom::CredentialType::FEDERATED;
    out->federation =
        static_cast<const blink::WebFederatedCredential&>(credential)
            .Provider();
    out->name =
        static_cast<const blink::WebFederatedCredential&>(credential).Name();
    out->icon =
        static_cast<const blink::WebFederatedCredential&>(credential).IconURL();
  }
}

std::unique_ptr<blink::WebCredential> CredentialInfoToWebCredential(
    const password_manager::mojom::blink::CredentialInfo& info) {
  switch (info.type) {
    case password_manager::mojom::CredentialType::FEDERATED:
      return std::make_unique<blink::WebFederatedCredential>(
          info.id, info.federation, info.name, info.icon);
    case password_manager::mojom::CredentialType::PASSWORD:
      return std::make_unique<blink::WebPasswordCredential>(
          info.id, info.password, info.name, info.icon);
    case password_manager::mojom::CredentialType::EMPTY:
      return nullptr;
  }

  NOTREACHED();
  return nullptr;
}

password_manager::mojom::CredentialMediationRequirement
GetCredentialMediationRequirementFromBlink(
    WebCredentialMediationRequirement mediation) {
  switch (mediation) {
    case WebCredentialMediationRequirement::kSilent:
      return password_manager::mojom::CredentialMediationRequirement::kSilent;
    case WebCredentialMediationRequirement::kOptional:
      return password_manager::mojom::CredentialMediationRequirement::kOptional;
    case WebCredentialMediationRequirement::kRequired:
      return password_manager::mojom::CredentialMediationRequirement::kRequired;
  }

  NOTREACHED();
  return password_manager::mojom::CredentialMediationRequirement::kOptional;
}

blink::WebCredentialManagerError GetWebCredentialManagerErrorFromMojo(
    password_manager::mojom::CredentialManagerError error) {
  switch (error) {
    case password_manager::mojom::CredentialManagerError::DISABLED:
      return blink::WebCredentialManagerError::
          kWebCredentialManagerDisabledError;
    case password_manager::mojom::CredentialManagerError::PENDINGREQUEST:
      return blink::WebCredentialManagerError::
          kWebCredentialManagerPendingRequestError;
    case password_manager::mojom::CredentialManagerError::
        PASSWORDSTOREUNAVAILABLE:
      return blink::WebCredentialManagerError::
          kWebCredentialManagerPasswordStoreUnavailableError;
    case password_manager::mojom::CredentialManagerError::UNKNOWN:
      return blink::WebCredentialManagerError::
          kWebCredentialManagerUnknownError;
    case password_manager::mojom::CredentialManagerError::SUCCESS:
      NOTREACHED();
      break;
  }

  NOTREACHED();
  return blink::WebCredentialManagerError::kWebCredentialManagerUnknownError;
}

// Takes ownership of CredentialManagerClient::NotificationCallbacks
// pointer. When the wrapper is destroyed, if |callbacks| is still alive
// its onError() will get called.
class NotificationCallbacksWrapper {
 public:
  explicit NotificationCallbacksWrapper(
      CredentialManagerClient::NotificationCallbacks* callbacks);

  ~NotificationCallbacksWrapper();

  void NotifySuccess();

 private:
  std::unique_ptr<CredentialManagerClient::NotificationCallbacks> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(NotificationCallbacksWrapper);
};

NotificationCallbacksWrapper::NotificationCallbacksWrapper(
    CredentialManagerClient::NotificationCallbacks* callbacks)
    : callbacks_(callbacks) {}

NotificationCallbacksWrapper::~NotificationCallbacksWrapper() {
  if (callbacks_)
    callbacks_->OnError(blink::kWebCredentialManagerUnknownError);
}

void NotificationCallbacksWrapper::NotifySuccess() {
  // Call onSuccess() and reset callbacks to avoid calling onError() in
  // destructor.
  if (callbacks_) {
    callbacks_->OnSuccess();
    callbacks_.reset();
  }
}

// Takes ownership of CredentialManagerClient::RequestCallbacks
// pointer. When the wrapper is destroyed, if |callbacks| is still alive
// its onError() will get called.
class RequestCallbacksWrapper {
 public:
  explicit RequestCallbacksWrapper(
      CredentialManagerClient::RequestCallbacks* callbacks);

  ~RequestCallbacksWrapper();

  void RespondToRequestCallback(
      password_manager::mojom::CredentialManagerError error,
      password_manager::mojom::blink::CredentialInfoPtr credential);

 private:
  std::unique_ptr<CredentialManagerClient::RequestCallbacks> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(RequestCallbacksWrapper);
};

RequestCallbacksWrapper::RequestCallbacksWrapper(
    CredentialManagerClient::RequestCallbacks* callbacks)
    : callbacks_(callbacks) {}

RequestCallbacksWrapper::~RequestCallbacksWrapper() {
  if (callbacks_)
    callbacks_->OnError(blink::kWebCredentialManagerUnknownError);
}

void RequestCallbacksWrapper::RespondToRequestCallback(
    password_manager::mojom::CredentialManagerError error,
    password_manager::mojom::blink::CredentialInfoPtr credential) {
  if (error == password_manager::mojom::CredentialManagerError::SUCCESS) {
    DCHECK(credential);
    if (callbacks_)
      callbacks_->OnSuccess(CredentialInfoToWebCredential(*credential));
  } else {
    DCHECK(!credential);
    if (callbacks_)
      callbacks_->OnError(GetWebCredentialManagerErrorFromMojo(error));
  }
  callbacks_.reset();
}

}  // namespace

CredentialManagerClient::CredentialManagerClient(Page& page)
    : Supplement<Page>(page) {}

CredentialManagerClient::~CredentialManagerClient() = default;

void CredentialManagerClient::Trace(blink::Visitor* visitor) {
  visitor->Trace(authentication_client_);
  Supplement<Page>::Trace(visitor);
}

// static
const char* CredentialManagerClient::SupplementName() {
  return "CredentialManagerClient";
}

// static
CredentialManagerClient* CredentialManagerClient::From(
    ExecutionContext* execution_context) {
  if (!execution_context->IsDocument() ||
      !ToDocument(execution_context)->GetPage())
    return nullptr;
  return From(ToDocument(execution_context)->GetPage());
}

// static
CredentialManagerClient* CredentialManagerClient::From(Page* page) {
  return static_cast<CredentialManagerClient*>(
      Supplement<Page>::From(page, SupplementName()));
}

void CredentialManagerClient::DispatchStore(const WebCredential& credential,
                                            NotificationCallbacks* callbacks) {
  DCHECK(callbacks);
  ConnectToMojoCMIfNeeded();
  if (!mojo_cm_service_)
    return;

  auto info = password_manager::mojom::blink::CredentialInfo::New();
  WebCredentialToCredentialInfo(credential, info.get());
  mojo_cm_service_->Store(
      std::move(info),
      base::BindOnce(
          &NotificationCallbacksWrapper::NotifySuccess,
          std::make_unique<NotificationCallbacksWrapper>(callbacks)));
}

void CredentialManagerClient::DispatchPreventSilentAccess(
    NotificationCallbacks* callbacks) {
  DCHECK(callbacks);
  ConnectToMojoCMIfNeeded();
  if (!mojo_cm_service_)
    return;

  mojo_cm_service_->PreventSilentAccess(base::BindOnce(
      &NotificationCallbacksWrapper::NotifySuccess,
      std::make_unique<NotificationCallbacksWrapper>(callbacks)));
}

void CredentialManagerClient::DispatchGet(
    WebCredentialMediationRequirement mediation,
    bool include_passwords,
    const WTF::Vector<KURL>& federations,
    RequestCallbacks* callbacks) {
  DCHECK(callbacks);
  ConnectToMojoCMIfNeeded();
  if (!mojo_cm_service_)
    return;

  mojo_cm_service_->Get(
      GetCredentialMediationRequirementFromBlink(mediation), include_passwords,
      federations,
      base::Bind(&RequestCallbacksWrapper::RespondToRequestCallback,
                 std::make_unique<RequestCallbacksWrapper>(callbacks)));
}

void CredentialManagerClient::DispatchMakeCredential(
    LocalFrame& frame,
    const MakePublicKeyCredentialOptions& options,
    std::unique_ptr<WebAuthenticationClient::PublicKeyCallbacks> callbacks) {
  if (!authentication_client_)
    authentication_client_ = new WebAuthenticationClient(frame);
  authentication_client_->DispatchMakeCredential(options, std::move(callbacks));
}

void CredentialManagerClient::ConnectToMojoCMIfNeeded() {
  if (mojo_cm_service_)
    return;

  LocalFrame* main_frame = ToLocalFrame(GetSupplementable()->MainFrame());
  main_frame->GetRemoteNavigationAssociatedInterfaces()->GetInterface(
      &mojo_cm_service_);

  // The remote end of the pipe will be forcibly closed by the browser side
  // after each main frame navigation. Set up an error handler to reset the
  // local end so that there will be an attempt at reestablishing the connection
  // at the next call to the API.
  mojo_cm_service_.set_connection_error_handler(base::Bind(
      &CredentialManagerClient::OnMojoConnectionError, base::Unretained(this)));
}

void CredentialManagerClient::OnMojoConnectionError() {
  mojo_cm_service_.reset();
}

void ProvideCredentialManagerClientTo(Page& page,
                                      CredentialManagerClient* client) {
  CredentialManagerClient::ProvideTo(
      page, CredentialManagerClient::SupplementName(), client);
}

}  // namespace blink
