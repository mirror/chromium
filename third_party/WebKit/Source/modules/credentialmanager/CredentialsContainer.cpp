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

namespace mojo {

using ::password_manager::mojom::blink::CredentialInfo;
using ::password_manager::mojom::blink::CredentialType;
using ::password_manager::mojom::blink::CredentialInfoPtr;

template <>
struct TypeConverter<CredentialInfoPtr, ::blink::Credential*> {
  static CredentialInfoPtr Convert(::blink::Credential* credential) {
    auto info = CredentialInfo::New();
    info->id = credential->id();
    if (credential->IsPasswordCredential()) {
      ::blink::PasswordCredential* password_credential =
          static_cast<::blink::PasswordCredential*>(credential);
      info->type = CredentialType::PASSWORD;
      info->password = password_credential->password();
      info->name = password_credential->name();
      info->icon = password_credential->iconURL();
    } else {
      DCHECK(credential->IsFederatedCredential());
      ::blink::FederatedCredential* federated_credential =
          static_cast<::blink::FederatedCredential*>(credential);
      info->type = CredentialType::FEDERATED;
      info->federation = federated_credential->GetProviderAsOrigin();
      info->name = federated_credential->name();
      info->icon = federated_credential->iconURL();
    }
    return info;
  }
};

template <>
struct TypeConverter<::blink::Credential*, CredentialInfoPtr> {
  static ::blink::Credential* Convert(const CredentialInfoPtr& info) {
    switch (info->type) {
      case CredentialType::FEDERATED:
        return ::blink::FederatedCredential::Create(info->id, info->federation,
                                                    info->name, info->icon);
      case CredentialType::PASSWORD:
        return ::blink::PasswordCredential::Create(info->id, info->password,
                                                   info->name, info->icon);
      case CredentialType::EMPTY:
        return nullptr;
    }
    NOTREACHED();
    return nullptr;
  }
};

}  // namespace mojo

namespace blink {

namespace {

using ::password_manager::mojom::blink::CredentialManagerError;
using ::password_manager::mojom::blink::CredentialInfo;
using ::password_manager::mojom::blink::CredentialInfoPtr;
using ::password_manager::mojom::blink::CredentialMediationRequirement;

void RejectDueToCredentialManagerError(ScriptPromiseResolver* resolver,
                                       CredentialManagerError reason) {
  switch (reason) {
    case CredentialManagerError::DISABLED:
      resolver->Reject(DOMException::Create(
          kInvalidStateError, "The credential manager is disabled."));
      break;
    case CredentialManagerError::PENDING_REQUEST:
      resolver->Reject(DOMException::Create(kInvalidStateError,
                                            "A 'get()' request is pending."));
      break;
    case CredentialManagerError::PASSWORD_STORE_UNAVAILABLE:
      resolver->Reject(DOMException::Create(
          kNotSupportedError, "The password store is unavailable."));
      break;
    case CredentialManagerError::NOT_ALLOWED:
      resolver->Reject(DOMException::Create(kNotAllowedError,
                                            "The operation is not allowed."));
      break;
    case CredentialManagerError::NOT_SUPPORTED:
      resolver->Reject(DOMException::Create(
          kNotSupportedError,
          "Parameters for this operation are not supported."));
      break;
    case CredentialManagerError::NOT_SECURE:
      resolver->Reject(DOMException::Create(kSecurityError,
                                            "The operation is insecure and "
                                            "is not allowed."));
      break;
    case CredentialManagerError::CANCELLED:
      resolver->Reject(DOMException::Create(
          kNotAllowedError, "The user cancelled the operation."));
      break;
    case CredentialManagerError::NOT_IMPLEMENTED:
      resolver->Reject(DOMException::Create(
          kNotAllowedError, "The operation is not implemented."));
      break;
    case CredentialManagerError::UNKNOWN:
    default:
      resolver->Reject(DOMException::Create(kNotReadableError,
                                            "An unknown error occurred while "
                                            "talking to the credential "
                                            "manager."));
      break;
  }
  NOTREACHED();
}

bool CheckBoilerplate(ScriptPromiseResolver* resolver) {
  Frame* frame = ToDocument(ExecutionContext::From(resolver->GetScriptState()))
                     ->GetFrame();
  if (!frame || frame != frame->Tree().Top()) {
    resolver->Reject(DOMException::Create(kSecurityError,
                                          "CredentialContainer methods may "
                                          "only be executed in a top-level "
                                          "document."));
    return false;
  }

  String error_message;
  if (!ExecutionContext::From(resolver->GetScriptState())
           ->IsSecureContext(error_message)) {
    resolver->Reject(DOMException::Create(kSecurityError, error_message));
    return false;
  }

  CredentialManagerProxy* client = CredentialManagerProxy::From(
      ExecutionContext::From(resolver->GetScriptState()));
  if (!client) {
    resolver->Reject(DOMException::Create(
        kInvalidStateError,
        "Could not establish connection to the credential manager."));
    return false;
  }

  return true;
}

bool IsIconURLInsecure(const Credential* credential) {
  auto is_insecure = [](const KURL& url) {
    return !url.IsEmpty() && !url.ProtocolIs("https");
  };
  if (credential->IsFederatedCredential()) {
    return is_insecure(
        static_cast<const FederatedCredential*>(credential)->iconURL());
  }
  if (credential->IsPasswordCredential()) {
    return is_insecure(
        static_cast<const PasswordCredential*>(credential)->iconURL());
  }
  return false;
}

// Owns a ScriptPromiseResolver and rejects when the instance goes out of scope,
// unless Release() is called to release ownership of the promise resolver.
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
    // TODO: Why do we need this check?
    Frame* frame =
        ToDocument(ExecutionContext::From(resolver_->GetScriptState()))
            ->GetFrame();
    SECURITY_CHECK(!frame || frame == frame->Tree().Top());
    return resolver_.Release();
  }

 private:
  Persistent<ScriptPromiseResolver> resolver_;
};

void OnOperationComplete(std::unique_ptr<AutoPromiseResolver> scoped_resolver) {
  DCHECK(scoped_resolver);
  ScriptPromiseResolver* resolver = scoped_resolver->Release();
  resolver->Resolve();
}

void OnGetOperationComplete(
    std::unique_ptr<AutoPromiseResolver> scoped_resolver,
    CredentialManagerError error,
    CredentialInfoPtr credential_info) {
  DCHECK(scoped_resolver);
  ScriptPromiseResolver* resolver = scoped_resolver->Release();
  if (error == CredentialManagerError::SUCCESS) {
    DCHECK(credential_info);
    UseCounter::Count(ExecutionContext::From(resolver->GetScriptState()),
                      WebFeature::kCredentialManagerGetReturnedCredential);
    resolver->Resolve(mojo::ConvertTo<Credential*>(std::move(credential_info)));
  } else {
    RejectDueToCredentialManagerError(resolver, error);
  }
}

class PublicKeyCallbacks : public WebAuthenticationClient::PublicKeyCallbacks {
  WTF_MAKE_NONCOPYABLE(PublicKeyCallbacks);

 public:
  explicit PublicKeyCallbacks(ScriptPromiseResolver* resolver)
      : resolver_(resolver) {}
  ~PublicKeyCallbacks() override { OnError(CredentialManagerError::UNKNOWN); }

  void OnSuccess(
      webauth::mojom::blink::PublicKeyCredentialInfoPtr credential) override {
    ExecutionContext* context =
        ExecutionContext::From(resolver_->GetScriptState());
    if (!context)
      return;
    Frame* frame = ToDocument(context)->GetFrame();
    SECURITY_CHECK(!frame || frame == frame->Tree().Top());

    if (!credential || !frame) {
      resolver_->Resolve();
      return;
    }

    if (credential->client_data_json.IsEmpty() ||
        credential->response->attestation_object.IsEmpty()) {
      resolver_->Resolve();
      return;
    }

    DOMArrayBuffer* client_data_buffer =
        VectorToDOMArrayBuffer(std::move(credential->client_data_json));
    DOMArrayBuffer* attestation_buffer = VectorToDOMArrayBuffer(
        std::move(credential->response->attestation_object));
    DOMArrayBuffer* raw_id =
        VectorToDOMArrayBuffer(std::move(credential->raw_id));
    AuthenticatorAttestationResponse* authenticator_response =
        AuthenticatorAttestationResponse::Create(client_data_buffer,
                                                 attestation_buffer);
    resolver_->Resolve(PublicKeyCredential::Create(credential->id, raw_id,
                                                   authenticator_response));
  }

  void OnError(CredentialManagerError reason) override {
    RejectDueToCredentialManagerError(resolver_, reason);
  }

 private:
  DOMArrayBuffer* VectorToDOMArrayBuffer(const Vector<uint8_t> buffer) {
    return DOMArrayBuffer::Create(static_cast<const void*>(buffer.data()),
                                  buffer.size());
  }

  const Persistent<ScriptPromiseResolver> resolver_;
};

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
  if (!CheckBoilerplate(resolver))
    return promise;

  ExecutionContext* context = ExecutionContext::From(script_state);
  // Set the default mediation option if none is provided.
  // If both 'unmediated' and 'mediation' are set log a warning if they are
  // contradicting.
  // Also sets 'mediation' appropriately when only 'unmediated' is set.
  // TODO(http://crbug.com/715077): Remove this when 'unmediated' is removed.
  String mediation = "optional";
  if (options.hasUnmediated() && !options.hasMediation()) {
    mediation = options.unmediated() ? "silent" : "optional";
    UseCounter::Count(
        context,
        WebFeature::kCredentialManagerCredentialRequestOptionsOnlyUnmediated);
  } else if (options.hasMediation()) {
    mediation = options.mediation();
    if (options.hasUnmediated() &&
        ((options.unmediated() && options.mediation() != "silent") ||
         (!options.unmediated() && options.mediation() != "optional"))) {
      context->AddConsoleMessage(ConsoleMessage::Create(
          kJSMessageSource, kWarningMessageLevel,
          "mediation: '" + options.mediation() + "' overrides unmediated: " +
              (options.unmediated() ? "true" : "false") + "."));
    }
  }

  Vector<KURL> providers;
  if (options.hasFederated() && options.federated().hasProviders()) {
    for (const auto& string : options.federated().providers()) {
      KURL url = KURL(NullURL(), string);
      if (url.IsValid())
        providers.push_back(std::move(url));
    }
  }

  CredentialMediationRequirement requirement;

  if (mediation == "silent") {
    UseCounter::Count(context,
                      WebFeature::kCredentialManagerGetMediationSilent);
    requirement = CredentialMediationRequirement::kSilent;
  } else if (mediation == "optional") {
    UseCounter::Count(context,
                      WebFeature::kCredentialManagerGetMediationOptional);
    requirement = CredentialMediationRequirement::kOptional;
  } else {
    DCHECK_EQ("required", mediation);
    UseCounter::Count(context,
                      WebFeature::kCredentialManagerGetMediationRequired);
    requirement = CredentialMediationRequirement::kRequired;
  }

  auto* credential_manager = CredentialManagerProxy::From(context)->Service();
  credential_manager->Get(
      requirement, options.password(), std::move(providers),
      ConvertToBaseCallback(
          WTF::Bind(&OnGetOperationComplete,
                    std::make_unique<AutoPromiseResolver>(resolver))));
  return promise;
}

ScriptPromise CredentialsContainer::store(ScriptState* script_state,
                                          Credential* credential) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  if (!CheckBoilerplate(resolver))
    return promise;

  if (IsIconURLInsecure(credential)) {
    resolver->Reject(DOMException::Create(kSecurityError,
                                          "'iconURL' should be a secure URL"));
    return promise;
  }

  auto* context = ExecutionContext::From(script_state);
  auto* credential_manager = CredentialManagerProxy::From(context)->Service();
  credential_manager->Store(
      CredentialInfo::From(credential),
      ConvertToBaseCallback(
          WTF::Bind(&OnOperationComplete,
                    std::make_unique<AutoPromiseResolver>(resolver))));
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
    resolver->Reject(DOMException::Create(kNotSupportedError,
                                          "Only exactly one of 'password', "
                                          "'federated', and 'publicKey' "
                                          "credential types are currently "
                                          "supported."));
    return promise;
  }

  if (options.hasPassword()) {
    if (options.password().IsPasswordCredentialData()) {
      resolver->Resolve(PasswordCredential::Create(
          options.password().GetAsPasswordCredentialData(), exception_state));
    } else {
      resolver->Resolve(PasswordCredential::Create(
          options.password().GetAsHTMLFormElement(), exception_state));
    }
  } else if (options.hasFederated()) {
    resolver->Resolve(
        FederatedCredential::Create(options.federated(), exception_state));
  } else {
    DCHECK(options.hasPublicKey());
    // Dispatch the publicKey credential operation.
    // TODO(https://crbug.com/740081): Eventually unify with CredMan's mojo.
    // TODO(engedy): Make frame checks more efficient in the refactor.
    // LocalFrame* frame =
    //     ToDocument(ExecutionContext::From(script_state))->GetFrame();
    // CredentialManagerProxy::From(ExecutionContext::From(script_state))
    //     ->DispatchMakeCredential(
    //         *frame, options.publicKey(),
    //         std::make_unique<PublicKeyCallbacks>(resolver));
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
  auto* credential_manager = CredentialManagerProxy::From(context)->Service();
  credential_manager->PreventSilentAccess(ConvertToBaseCallback(WTF::Bind(
      &OnOperationComplete, std::make_unique<AutoPromiseResolver>(resolver)));
  return promise;
}

ScriptPromise CredentialsContainer::requireUserMediation(
    ScriptState* script_state) {
  return preventSilentAccess(script_state);
}

}  // namespace blink
