// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialsContainer.h"

#include <memory>
#include <utility>
#include "bindings/core/v8/Dictionary.h"
#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/Frame.h"
#include "core/frame/UseCounter.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/page/FrameTree.h"
#include "modules/credentialmanager/AuthenticatorAttestationResponse.h"
#include "modules/credentialmanager/AuthenticatorResponse.h"
#include "modules/credentialmanager/Credential.h"
#include "modules/credentialmanager/CredentialCreationOptions.h"
#include "modules/credentialmanager/CredentialManagerProxy.h"
#include "modules/credentialmanager/CredentialManagerTypeConverters.h"
#include "modules/credentialmanager/CredentialRequestOptions.h"
#include "modules/credentialmanager/FederatedCredential.h"
#include "modules/credentialmanager/FederatedCredentialRequestOptions.h"
#include "modules/credentialmanager/MakeCredentialOptions.h"
#include "modules/credentialmanager/PasswordCredential.h"
#include "modules/credentialmanager/PublicKeyCredential.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/Functional.h"
#include "platform/wtf/PtrUtil.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

namespace {

using ::password_manager::mojom::blink::CredentialManagerError;
using ::password_manager::mojom::blink::CredentialInfo;
using ::password_manager::mojom::blink::CredentialInfoPtr;
using ::password_manager::mojom::blink::CredentialMediationRequirement;
using ::webauth::mojom::blink::AuthenticatorStatus;
using MojoMakeCredentialOptions =
    ::webauth::mojom::blink::MakeCredentialOptions;
using ::webauth::mojom::blink::PublicKeyCredentialInfoPtr;

// Owns a persistent handle to a ScriptPromiseResolver and rejects the promise
// when the instance goes out of scope unless Release() is called beforehand.
class AutoPromiseResolver {
  WTF_MAKE_NONCOPYABLE(AutoPromiseResolver);

 public:
  explicit AutoPromiseResolver(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}

  ~AutoPromiseResolver() {
    if (resolver_)
      resolver_->Reject();
  }

  ScriptPromiseResolver* Release() {
    DCHECK(resolver_);
    // TODO: if !frame, the promise should be resolved with undefined.
    return resolver_.Release();
  }

 private:
  Persistent<ScriptPromiseResolver> resolver_;
};

bool IsSameOriginWithAncestors(Frame* frame) {
  if (!frame)
    return true;

  Frame* current = frame;
  SecurityOrigin* origin = frame->GetSecurityContext()->GetSecurityOrigin();
  while (current->Tree().Parent()) {
    current = current->Tree().Parent();
    if (!origin->CanAccess(current->GetSecurityContext()->GetSecurityOrigin()))
      return false;
  }
  return true;
}

DOMException* CredentialManagerErrorToDOMException(
    CredentialManagerError reason) {
  switch (reason) {
    case CredentialManagerError::DISABLED:
      return DOMException::Create(kInvalidStateError,
                                  "The credential manager is disabled.");
    case CredentialManagerError::PENDING_REQUEST:
      return DOMException::Create(kInvalidStateError,
                                  "A 'get()' request is pending.");
    case CredentialManagerError::PASSWORD_STORE_UNAVAILABLE:
      return DOMException::Create(kNotSupportedError,
                                  "The password store is unavailable.");
    case CredentialManagerError::NOT_ALLOWED:
      return DOMException::Create(kNotAllowedError,
                                  "The operation is not allowed.");
    case CredentialManagerError::NOT_SUPPORTED:
      return DOMException::Create(
          kNotSupportedError,
          "Parameters for this operation are not supported.");
    case CredentialManagerError::NOT_SECURE:
      return DOMException::Create(
          kSecurityError, "The operation is insecure and is not allowed.");
    case CredentialManagerError::CANCELLED:
      return DOMException::Create(kNotAllowedError,
                                  "The user cancelled the operation.");
    case CredentialManagerError::NOT_IMPLEMENTED:
      return DOMException::Create(kNotAllowedError,
                                  "The operation is not implemented.");
    case CredentialManagerError::UNKNOWN:
      return DOMException::Create(
          kNotReadableError,
          "An unknown error occurred while talking to the credential manager.");
    case CredentialManagerError::SUCCESS:
      NOTREACHED();
      break;
  }
  return nullptr;
}

CredentialManagerError GetWebCredentialManagerErrorFromStatus(
    webauth::mojom::blink::AuthenticatorStatus status) {
  switch (status) {
    case webauth::mojom::blink::AuthenticatorStatus::NOT_IMPLEMENTED:
      return CredentialManagerError::NOT_IMPLEMENTED;
    case webauth::mojom::blink::AuthenticatorStatus::NOT_ALLOWED_ERROR:
      return CredentialManagerError::NOT_ALLOWED;
    case webauth::mojom::blink::AuthenticatorStatus::NOT_SUPPORTED_ERROR:
      return CredentialManagerError::NOT_SUPPORTED;
    case webauth::mojom::blink::AuthenticatorStatus::SECURITY_ERROR:
      return CredentialManagerError::NOT_SECURE;
    case webauth::mojom::blink::AuthenticatorStatus::UNKNOWN_ERROR:
      return CredentialManagerError::UNKNOWN;
    case webauth::mojom::blink::AuthenticatorStatus::CANCELLED:
      return CredentialManagerError::CANCELLED;
    case webauth::mojom::blink::AuthenticatorStatus::SUCCESS:
      NOTREACHED();
      break;
  };

  NOTREACHED();
  return CredentialManagerError::UNKNOWN;
}

bool CheckBoilerplate(ScriptPromiseResolver* resolver) {
  String error_message;
  if (!ExecutionContext::From(resolver->GetScriptState())
           ->IsSecureContext(error_message)) {
    resolver->Reject(DOMException::Create(kSecurityError, error_message));
    return false;
  }

  CredentialManagerProxy* proxy = CredentialManagerProxy::From(
      ExecutionContext::From(resolver->GetScriptState()));
  if (!proxy) {
    resolver->Reject(DOMException::Create(
        kInvalidStateError,
        "Could not establish connection to the credential manager."));
    return false;
  }

  return true;
}

bool IsIconURLInsecure(const Credential* credential) {
  if (!credential->IsPasswordCredential() &&
      credential->IsFederatedCredential())
    return false;

  const KURL& url =
      credential->IsFederatedCredential()
          ? static_cast<const FederatedCredential*>(credential)->iconURL()
          : static_cast<const PasswordCredential*>(credential)->iconURL();
  return !url.IsEmpty() && !url.ProtocolIs("https");
}

DOMArrayBuffer* VectorToDOMArrayBuffer(const Vector<uint8_t> buffer) {
  return DOMArrayBuffer::Create(static_cast<const void*>(buffer.data()),
                                buffer.size());
}

void OnStoreComplete(std::unique_ptr<AutoPromiseResolver> scoped_resolver) {
  DCHECK(scoped_resolver);
  ScriptPromiseResolver* resolver = scoped_resolver->Release();
  resolver->Resolve();
}

void OnPreventSilentAccessComplete(
    std::unique_ptr<AutoPromiseResolver> scoped_resolver) {
  DCHECK(scoped_resolver);
  ScriptPromiseResolver* resolver = scoped_resolver->Release();
  resolver->Resolve();
}

void OnGetComplete(std::unique_ptr<AutoPromiseResolver> scoped_resolver,
                   CredentialManagerError error,
                   CredentialInfoPtr credential_info) {
  DCHECK(scoped_resolver);
  ScriptPromiseResolver* resolver = scoped_resolver->Release();
  // TODO: Figure out if same origin requirement needs to be checked.
  if (error == CredentialManagerError::SUCCESS) {
    DCHECK(credential_info);
    UseCounter::Count(ExecutionContext::From(resolver->GetScriptState()),
                      WebFeature::kCredentialManagerGetReturnedCredential);
    resolver->Resolve(mojo::ConvertTo<Credential*>(std::move(credential_info)));
  } else {
    DCHECK(!credential_info);
    resolver->Reject(CredentialManagerErrorToDOMException(error));
  }
}

void OnMakePublicKeyCredentialComplete(
    std::unique_ptr<AutoPromiseResolver> scoped_resolver,
    AuthenticatorStatus status,
    PublicKeyCredentialInfoPtr credential) {
  DCHECK(scoped_resolver);
  ScriptPromiseResolver* resolver = scoped_resolver->Release();
  if (status == AuthenticatorStatus::SUCCESS) {
    DCHECK(credential);
    DCHECK(!credential->client_data_json.IsEmpty());
    DCHECK(!credential->response->attestation_object.IsEmpty());
    DOMArrayBuffer* client_data_buffer =
        VectorToDOMArrayBuffer(std::move(credential->client_data_json));
    DOMArrayBuffer* attestation_buffer = VectorToDOMArrayBuffer(
        std::move(credential->response->attestation_object));
    DOMArrayBuffer* raw_id =
        VectorToDOMArrayBuffer(std::move(credential->raw_id));
    AuthenticatorAttestationResponse* authenticator_response =
        AuthenticatorAttestationResponse::Create(client_data_buffer,
                                                 attestation_buffer);
    resolver->Resolve(PublicKeyCredential::Create(credential->id, raw_id,
                                                  authenticator_response));
  } else {
    DCHECK(!credential);
    resolver->Reject(CredentialManagerErrorToDOMException(
        GetWebCredentialManagerErrorFromStatus(status)));
  }
}

}  // namespace

CredentialsContainer* CredentialsContainer::Create() {
  return new CredentialsContainer();
}

CredentialsContainer::CredentialsContainer() {}

ScriptPromise CredentialsContainer::get(
    ScriptState* script_state,
    const CredentialRequestOptions& options) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  ExecutionContext* context = ExecutionContext::From(script_state);

  Frame* frame = ToDocument(context)->GetFrame();
  if ((options.hasPassword() || options.hasFederated()) &&
      !IsSameOriginWithAncestors(frame)) {
    resolver->Reject(DOMException::Create(
        kNotAllowedError,
        "`PasswordCredential` and `FederatedCredential` objects may only be "
        "retrieved in a document which is same-origin with all of its "
        "ancestors."));
    return promise;
  }

  if (!CheckBoilerplate(resolver))
    return promise;

  Vector<KURL> providers;
  if (options.hasFederated() && options.federated().hasProviders()) {
    for (const auto& string : options.federated().providers()) {
      KURL url = KURL(NullURL(), string);
      if (url.IsValid())
        providers.push_back(std::move(url));
    }
  }

  CredentialMediationRequirement requirement;
  if (options.mediation() == "silent") {
    UseCounter::Count(context,
                      WebFeature::kCredentialManagerGetMediationSilent);
    requirement = CredentialMediationRequirement::kSilent;
  } else if (options.mediation() == "optional") {
    UseCounter::Count(context,
                      WebFeature::kCredentialManagerGetMediationOptional);
    requirement = CredentialMediationRequirement::kOptional;
  } else {
    DCHECK_EQ("required", options.mediation());
    UseCounter::Count(context,
                      WebFeature::kCredentialManagerGetMediationRequired);
    requirement = CredentialMediationRequirement::kRequired;
  }

  CredentialManagerProxy::From(context)->CredentialManager()->Get(
      requirement, options.password(), std::move(providers),
      ConvertToBaseCallback(WTF::Bind(
          &OnGetComplete,
          WTF::Passed(std::make_unique<AutoPromiseResolver>(resolver)))));

  return promise;
}

ScriptPromise CredentialsContainer::store(ScriptState* script_state,
                                          Credential* credential) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  Frame* frame = ToDocument(ExecutionContext::From(script_state))->GetFrame();
  if ((credential->type() == "password" || credential->type() == "federated") &&
      !IsSameOriginWithAncestors(frame)) {
    resolver->Reject(
        DOMException::Create(kNotAllowedError,
                             "`PasswordCredential` and `FederatedCredential` "
                             "objects may only be stored in a document which "
                             "is same-origin with all of its ancestors."));
    return promise;
  }

  if (!CheckBoilerplate(resolver))
    return promise;

  if (IsIconURLInsecure(credential)) {
    resolver->Reject(DOMException::Create(kSecurityError,
                                          "'iconURL' should be a secure URL"));
    return promise;
  }

  auto* context = ExecutionContext::From(script_state);
  CredentialManagerProxy::From(context)->CredentialManager()->Store(
      CredentialInfo::From(credential),
      ConvertToBaseCallback(WTF::Bind(
          &OnStoreComplete,
          WTF::Passed(std::make_unique<AutoPromiseResolver>(resolver)))));

  return promise;
}

ScriptPromise CredentialsContainer::create(
    ScriptState* script_state,
    const CredentialCreationOptions& options,
    ExceptionState& exception_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  if (!CheckBoilerplate(resolver))
    return promise;

  if ((options.hasPassword() + options.hasFederated() +
       options.hasPublicKey()) != 1) {
    resolver->Reject(DOMException::Create(
        kNotSupportedError,
        "Only exactly one of 'password', 'federated', and 'publicKey' "
        "credential types are currently supported."));
  } else if (options.hasPassword()) {
    resolver->Resolve(
        options.password().IsPasswordCredentialData()
            ? PasswordCredential::Create(
                  options.password().GetAsPasswordCredentialData(),
                  exception_state)
            : PasswordCredential::Create(
                  options.password().GetAsHTMLFormElement(), exception_state));
  } else if (options.hasFederated()) {
    resolver->Resolve(
        FederatedCredential::Create(options.federated(), exception_state));
  } else {
    DCHECK(options.hasPublicKey());
    auto mojo_options = MojoMakeCredentialOptions::From(options.publicKey());
    if (mojo_options) {
      auto* context = ExecutionContext::From(script_state);
      CredentialManagerProxy::From(context)->Authenticator()->MakeCredential(
          std::move(mojo_options),
          ConvertToBaseCallback(WTF::Bind(
              &OnMakePublicKeyCredentialComplete,
              WTF::Passed(std::make_unique<AutoPromiseResolver>(resolver)))));
    } else {
      resolver->Reject(CredentialManagerErrorToDOMException(
          CredentialManagerError::NOT_SUPPORTED));
    }
  }

  return promise;
}

ScriptPromise CredentialsContainer::preventSilentAccess(
    ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  if (!CheckBoilerplate(resolver))
    return promise;

  auto* context = ExecutionContext::From(script_state);
  CredentialManagerProxy::From(context)
      ->CredentialManager()
      ->PreventSilentAccess(ConvertToBaseCallback(WTF::Bind(
          &OnPreventSilentAccessComplete,
          WTF::Passed(std::make_unique<AutoPromiseResolver>(resolver)))));

  return promise;
}

}  // namespace blink
