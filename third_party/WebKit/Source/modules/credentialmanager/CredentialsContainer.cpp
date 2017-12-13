// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialsContainer.h"

#include <memory>
#include <utility>

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
#include "core/typed_arrays/DOMArrayBuffer.h"
#include "modules/credentialmanager/AuthenticatorAttestationResponse.h"
#include "modules/credentialmanager/AuthenticatorResponse.h"
#include "modules/credentialmanager/Credential.h"
#include "modules/credentialmanager/CredentialCreationOptions.h"
#include "modules/credentialmanager/CredentialManagerProxy.h"
#include "modules/credentialmanager/CredentialManagerTypeConverters.h"
#include "modules/credentialmanager/CredentialRequestOptions.h"
#include "modules/credentialmanager/FederatedCredential.h"
#include "modules/credentialmanager/FederatedCredentialRequestOptions.h"
#include "modules/credentialmanager/MakePublicKeyCredentialOptions.h"
#include "modules/credentialmanager/PasswordCredential.h"
#include "modules/credentialmanager/PublicKeyCredential.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/Functional.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace blink {

namespace {

using ::password_manager::mojom::blink::CredentialManagerError;
using ::password_manager::mojom::blink::CredentialInfo;
using ::password_manager::mojom::blink::CredentialInfoPtr;
using ::password_manager::mojom::blink::CredentialMediationRequirement;
using ::webauth::mojom::blink::AuthenticatorStatus;
using MojoMakePublicKeyCredentialOptions =
    ::webauth::mojom::blink::MakePublicKeyCredentialOptions;
using ::webauth::mojom::blink::PublicKeyCredentialInfoPtr;

enum class RequiredOriginType { kSecure, kSecureAndSameAsAncestors };

// Owns a persistent handle to a ScriptPromiseResolver and rejects the promise
// when the instance goes out of scope unless Release() is called.
class ScopedPromiseResolver {
  WTF_MAKE_NONCOPYABLE(ScopedPromiseResolver);

 public:
  explicit ScopedPromiseResolver(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}

  ~ScopedPromiseResolver() {
    if (resolver_)
      resolver_->Reject();
  }

  ScriptPromiseResolver* Release() { return resolver_.Release(); }

 private:
  Persistent<ScriptPromiseResolver> resolver_;
};

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
      return DOMException::Create(kNotReadableError,
                                  "An unknown error occurred while talking "
                                  "to the credential manager.");
    case CredentialManagerError::SUCCESS:
      NOTREACHED();
      break;
  }
  return nullptr;
}

bool IsSameOriginWithAncestors(const Frame* frame) {
  DCHECK(frame);
  const Frame* current = frame;
  const SecurityOrigin* origin =
      frame->GetSecurityContext()->GetSecurityOrigin();
  while (current->Tree().Parent()) {
    current = current->Tree().Parent();
    if (!origin->CanAccess(current->GetSecurityContext()->GetSecurityOrigin()))
      return false;
  }
  return true;
}

bool CheckSecurityRequirementsBeforeRequest(
    ScriptPromiseResolver* resolver,
    RequiredOriginType required_origin_type) {
  const auto* context = ExecutionContext::From(resolver->GetScriptState());

  String error_message;
  if (!context->IsSecureContext(error_message)) {
    resolver->Reject(DOMException::Create(kSecurityError, error_message));
    return false;
  }

  auto* document = ToDocumentOrNull(context);
  if (!document || !document->GetFrame()) {
    resolver->Reject(DOMException::Create(kNotAllowedError,
                                          "The Credential Management API is "
                                          "unavailable in the current browsing "
                                          "context."));
    return false;
  }

  if (required_origin_type == RequiredOriginType::kSecureAndSameAsAncestors &&
      !IsSameOriginWithAncestors(document->GetFrame())) {
    resolver->Reject(DOMException::Create(
        kNotAllowedError,
        "`PasswordCredential` and `FederatedCredential` objects may only be "
        "stored/retrieved in a document which is same-origin with all of its "
        "ancestors."));
    return false;
  }

  return true;
}

// Returns false if the context is no longer valid, in which case it is okay to
// let the |resolver| get destroyed without getting resolved.
bool CheckSecurityRequirementsBeforeResponse(
    ScriptPromiseResolver* resolver,
    RequiredOriginType require_origin) {
  if (!resolver->GetScriptState()->ContextIsValid())
    return false;

  const auto* context = ExecutionContext::From(resolver->GetScriptState());
  const auto* document = ToDocumentOrNull(context);
  SECURITY_CHECK(document);
  SECURITY_CHECK(document->IsSecureContext());
  SECURITY_CHECK(document->GetFrame());
  SECURITY_CHECK(require_origin !=
                     RequiredOriginType::kSecureAndSameAsAncestors ||
                 IsSameOriginWithAncestors(document->GetFrame()));
  return true;
}

// Checks if the icon URL of |credential| is an a-priori authenticated URL.
// https://w3c.github.io/webappsec-credential-management/#dom-credentialuserdata-iconurl
bool IsIconURLSecure(const Credential* credential) {
  if (!credential->IsPasswordCredential() &&
      !credential->IsFederatedCredential()) {
    DCHECK(credential->IsPublicKeyCredential());
    return true;
  }

  const KURL& url =
      credential->IsFederatedCredential()
          ? static_cast<const FederatedCredential*>(credential)->iconURL()
          : static_cast<const PasswordCredential*>(credential)->iconURL();
  return url.IsEmpty() || url.ProtocolIs("https");
}

void OnStoreComplete(std::unique_ptr<ScopedPromiseResolver> scoped_resolver,
                     RequiredOriginType required_origin_type) {
  auto* resolver = scoped_resolver->Release();
  if (!CheckSecurityRequirementsBeforeResponse(resolver, required_origin_type))
    return;

  resolver->Resolve();
}

void OnPreventSilentAccessComplete(
    std::unique_ptr<ScopedPromiseResolver> scoped_resolver) {
  auto* resolver = scoped_resolver->Release();
  const auto required_origin_type = RequiredOriginType::kSecure;
  if (!CheckSecurityRequirementsBeforeResponse(resolver, required_origin_type))
    return;

  resolver->Resolve();
}

void OnGetComplete(std::unique_ptr<ScopedPromiseResolver> scoped_resolver,
                   RequiredOriginType required_origin_type,
                   CredentialManagerError error,
                   CredentialInfoPtr credential_info) {
  auto* resolver = scoped_resolver->Release();
  if (!CheckSecurityRequirementsBeforeResponse(resolver, required_origin_type))
    return;

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

DOMArrayBuffer* VectorToDOMArrayBuffer(const Vector<uint8_t> buffer) {
  return DOMArrayBuffer::Create(static_cast<const void*>(buffer.data()),
                                buffer.size());
}

void OnMakePublicKeyCredentialComplete(
    std::unique_ptr<ScopedPromiseResolver> scoped_resolver,
    AuthenticatorStatus status,
    PublicKeyCredentialInfoPtr credential) {
  auto* resolver = scoped_resolver->Release();
  const auto required_origin_type = RequiredOriginType::kSecure;
  if (!CheckSecurityRequirementsBeforeResponse(resolver, required_origin_type))
    return;

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
        mojo::ConvertTo<CredentialManagerError>(status)));
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

  auto required_origin_type =
      options.hasFederated() || options.hasPassword()
          ? RequiredOriginType::kSecureAndSameAsAncestors
          : RequiredOriginType::kSecure;
  if (!CheckSecurityRequirementsBeforeRequest(resolver, required_origin_type))
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
    UseCounter::Count(ExecutionContext::From(script_state),
                      WebFeature::kCredentialManagerGetMediationSilent);
    requirement = CredentialMediationRequirement::kSilent;
  } else if (options.mediation() == "optional") {
    UseCounter::Count(ExecutionContext::From(script_state),
                      WebFeature::kCredentialManagerGetMediationOptional);
    requirement = CredentialMediationRequirement::kOptional;
  } else {
    DCHECK_EQ("required", options.mediation());
    UseCounter::Count(ExecutionContext::From(script_state),
                      WebFeature::kCredentialManagerGetMediationRequired);
    requirement = CredentialMediationRequirement::kRequired;
  }

  auto* credential_manager =
      CredentialManagerProxy::From(script_state)->CredentialManager();
  credential_manager->Get(
      requirement, options.password(), std::move(providers),
      WTF::Bind(&OnGetComplete,
                WTF::Passed(std::make_unique<ScopedPromiseResolver>(resolver)),
                required_origin_type));

  return promise;
}

ScriptPromise CredentialsContainer::store(ScriptState* script_state,
                                          Credential* credential) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  auto required_origin_type =
      credential->IsFederatedCredential() || credential->IsPasswordCredential()
          ? RequiredOriginType::kSecureAndSameAsAncestors
          : RequiredOriginType::kSecure;
  if (!CheckSecurityRequirementsBeforeRequest(resolver, required_origin_type))
    return promise;

  if (!(credential->IsFederatedCredential() ||
        credential->IsPasswordCredential())) {
    resolver->Reject(DOMException::Create(
        kNotSupportedError,
        "Store operation not permitted for PublicKey credentials."));
  }

  if (!IsIconURLSecure(credential)) {
    resolver->Reject(DOMException::Create(kSecurityError,
                                          "'iconURL' should be a secure URL"));
    return promise;
  }

  auto* credential_manager =
      CredentialManagerProxy::From(script_state)->CredentialManager();
  credential_manager->Store(
      CredentialInfo::From(credential),
      WTF::Bind(&OnStoreComplete,
                WTF::Passed(std::make_unique<ScopedPromiseResolver>(resolver)),
                required_origin_type));

  return promise;
}

ScriptPromise CredentialsContainer::create(
    ScriptState* script_state,
    const CredentialCreationOptions& options,
    ExceptionState& exception_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  const auto required_origin_type = RequiredOriginType::kSecure;
  if (!CheckSecurityRequirementsBeforeRequest(resolver, required_origin_type))
    return promise;

  if ((options.hasPassword() + options.hasFederated() +
       options.hasPublicKey()) != 1) {
    resolver->Reject(DOMException::Create(
        kNotSupportedError,
        "Only exactly one of 'password', 'federated', and 'publicKey' "
        "credential types are currently supported."));
    return promise;
  }

  if (options.hasPassword()) {
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
    auto mojo_options =
        MojoMakePublicKeyCredentialOptions::From(options.publicKey());
    if (mojo_options) {
      auto* authenticator =
          CredentialManagerProxy::From(script_state)->Authenticator();
      authenticator->MakeCredential(
          std::move(mojo_options),
          WTF::Bind(
              &OnMakePublicKeyCredentialComplete,
              WTF::Passed(std::make_unique<ScopedPromiseResolver>(resolver))));
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
  const auto required_origin_type = RequiredOriginType::kSecure;
  if (!CheckSecurityRequirementsBeforeRequest(resolver, required_origin_type))
    return promise;

  auto* credential_manager =
      CredentialManagerProxy::From(script_state)->CredentialManager();
  credential_manager->PreventSilentAccess(WTF::Bind(
      &OnPreventSilentAccessComplete,
      WTF::Passed(std::make_unique<ScopedPromiseResolver>(resolver))));

  return promise;
}

}  // namespace blink
