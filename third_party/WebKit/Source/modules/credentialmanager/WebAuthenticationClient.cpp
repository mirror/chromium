// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/WebAuthenticationClient.h"

#include "bindings/core/v8/ArrayBufferOrArrayBufferView.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/LocalFrame.h"
#include "core/typed_arrays/DOMArrayBuffer.h"
#include "modules/credentialmanager/AuthenticatorAttestationResponse.h"
#include "modules/credentialmanager/MakeCredentialOptions.h"
#include "public/platform/InterfaceProvider.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {
typedef ArrayBufferOrArrayBufferView BufferSource;

namespace {
using PublicKeyCallbacks = WebAuthenticationClient::PublicKeyCallbacks;

// Takes ownership of PublicKeyCallbacks pointer.
// When the wrapper is destroyed - if |callbacks| is still alive -
// its onError() will get called.
class PublicKeyCallbacksWrapper {
 public:
  explicit PublicKeyCallbacksWrapper(PublicKeyCallbacks*);

  ~PublicKeyCallbacksWrapper();

  void NotifySuccess(webauth::mojom::blink::PublicKeyCredentialInfoPtr);
  void NotifyError(blink::WebCredentialManagerError);

 private:
  std::unique_ptr<PublicKeyCallbacks> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(PublicKeyCallbacksWrapper);
};

PublicKeyCallbacksWrapper::PublicKeyCallbacksWrapper(
    PublicKeyCallbacks* callbacks)
    : callbacks_(callbacks) {}

PublicKeyCallbacksWrapper::~PublicKeyCallbacksWrapper() {
  if (callbacks_)
    callbacks_->OnError(blink::kWebCredentialManagerUnknownError);
}

void PublicKeyCallbacksWrapper::NotifySuccess(
    webauth::mojom::blink::PublicKeyCredentialInfoPtr credential) {
  // Call onSuccess() and reset callbacks to avoid calling onError() in
  // destructor.
  if (callbacks_) {
    callbacks_->OnSuccess(std::move(credential));
    callbacks_.reset();
  }
}

void PublicKeyCallbacksWrapper::NotifyError(
    blink::WebCredentialManagerError error) {
  if (callbacks_) {
    callbacks_->OnError(error);
    callbacks_.reset();
  }
}

WebCredentialManagerError GetWebCredentialManagerErrorFromStatus(
    webauth::mojom::blink::AuthenticatorStatus status) {
  switch (status) {
    case webauth::mojom::blink::AuthenticatorStatus::NOT_IMPLEMENTED:
      return blink::kWebCredentialManagerNotImplementedError;
    case webauth::mojom::blink::AuthenticatorStatus::NOT_ALLOWED_ERROR:
      return WebCredentialManagerError::kWebCredentialManagerNotAllowedError;
    case webauth::mojom::blink::AuthenticatorStatus::NOT_SUPPORTED_ERROR:
      return WebCredentialManagerError::kWebCredentialManagerNotSupportedError;
    case webauth::mojom::blink::AuthenticatorStatus::SECURITY_ERROR:
      return WebCredentialManagerError::kWebCredentialManagerSecurityError;
    case webauth::mojom::blink::AuthenticatorStatus::UNKNOWN_ERROR:
      return WebCredentialManagerError::kWebCredentialManagerUnknownError;
    case webauth::mojom::blink::AuthenticatorStatus::CANCELLED:
      return WebCredentialManagerError::kWebCredentialManagerCancelledError;
    case webauth::mojom::blink::AuthenticatorStatus::SUCCESS:
      NOTREACHED();
      break;
  };

  NOTREACHED();
  return blink::WebCredentialManagerError::kWebCredentialManagerUnknownError;
}

void RespondToPublicKeyCallback(
    PublicKeyCallbacksWrapper* callbacks_wrapper,
    webauth::mojom::blink::AuthenticatorStatus status,
    webauth::mojom::blink::PublicKeyCredentialInfoPtr credential) {
  if (status != webauth::mojom::AuthenticatorStatus::SUCCESS) {
    DCHECK(!credential);
    callbacks_wrapper->NotifyError(
        GetWebCredentialManagerErrorFromStatus(status));
    return;
  }

  // Ensure we have an AuthenticatorAttestationResponse
  DCHECK(credential);
  DCHECK(!credential->client_data_json.IsEmpty());
  DCHECK(!credential->response->attestation_object.IsEmpty());
  callbacks_wrapper->NotifySuccess(std::move(credential));
}
}  // namespace

}  // namespace blink

namespace {
// Time to wait for an authenticator to successfully complete an operation.
constexpr WTF::TimeDelta kAdjustedTimeoutLower = WTF::TimeDelta::FromMinutes(1);
constexpr WTF::TimeDelta kAdjustedTimeoutUpper = WTF::TimeDelta::FromMinutes(2);
}  // namespace

namespace mojo {
using webauth::mojom::blink::AuthenticatorStatus;
using webauth::mojom::blink::MakeCredentialOptionsPtr;
using webauth::mojom::blink::PublicKeyCredentialEntityPtr;
using webauth::mojom::blink::PublicKeyCredentialParametersPtr;
using webauth::mojom::blink::PublicKeyCredentialType;
using webauth::mojom::blink::AuthenticatorTransport;

// TODO(kpaulhamus): Make this a TypeConverter
Vector<uint8_t> ConvertBufferSource(const blink::BufferSource& buffer) {
  DCHECK(!buffer.isNull());
  Vector<uint8_t> vector;
  if (buffer.isArrayBuffer()) {
    vector.Append(static_cast<uint8_t*>(buffer.getAsArrayBuffer()->Data()),
                  buffer.getAsArrayBuffer()->ByteLength());
  } else {
    DCHECK(buffer.isArrayBufferView());
    vector.Append(static_cast<uint8_t*>(
                      buffer.getAsArrayBufferView().View()->BaseAddress()),
                  buffer.getAsArrayBufferView().View()->byteLength());
  }
  return vector;
}

// TODO(kpaulhamus): Make this a TypeConverter
PublicKeyCredentialType ConvertPublicKeyCredentialType(const String& type) {
  if (type == "public-key")
    return PublicKeyCredentialType::PUBLIC_KEY;
  NOTREACHED();
  return PublicKeyCredentialType::PUBLIC_KEY;
}

// TODO(kpaulhamus): Make this a TypeConverter
AuthenticatorTransport ConvertTransport(const String& transport) {
  if (transport == "usb")
    return AuthenticatorTransport::USB;
  if (transport == "nfc")
    return AuthenticatorTransport::NFC;
  if (transport == "ble")
    return AuthenticatorTransport::BLE;
  NOTREACHED();
  return AuthenticatorTransport::USB;
}

// TODO(kpaulhamus): Make this a TypeConverter
PublicKeyCredentialEntityPtr ConvertPublicKeyCredentialUserEntity(
    const blink::PublicKeyCredentialUserEntity& user) {
  auto entity = webauth::mojom::blink::PublicKeyCredentialEntity::New();
  entity->id = user.id();
  entity->name = user.name();
  if (user.hasIcon()) {
    entity->icon = blink::KURL(blink::KURL(), user.icon());
  }
  entity->display_name = user.displayName();
  return entity;
}

// TODO(kpaulhamus): Make this a TypeConverter
PublicKeyCredentialEntityPtr ConvertPublicKeyCredentialEntity(
    const blink::PublicKeyCredentialEntity& rp) {
  auto entity = webauth::mojom::blink::PublicKeyCredentialEntity::New();
  entity->id = rp.id();
  entity->name = rp.name();
  if (rp.hasIcon()) {
    entity->icon = blink::KURL(blink::KURL(), rp.icon());
  }
  return entity;
}

// TODO(kpaulhamus): Make this a TypeConverter
PublicKeyCredentialParametersPtr ConvertPublicKeyCredentialParameters(
    const blink::PublicKeyCredentialParameters parameter) {
  auto mojo_parameter =
      webauth::mojom::blink::PublicKeyCredentialParameters::New();
  mojo_parameter->type = ConvertPublicKeyCredentialType(parameter.type());
  // TODO(kpaulhamus): add AlgorithmIdentifier
  return mojo_parameter;
}

// TODO(kpaulhamus): Make this a TypeConverter
MakeCredentialOptionsPtr ConvertMakeCredentialOptions(
    const blink::MakeCredentialOptions options) {
  auto mojo_options = webauth::mojom::blink::MakeCredentialOptions::New();
  mojo_options->relying_party = ConvertPublicKeyCredentialEntity(options.rp());
  mojo_options->user = ConvertPublicKeyCredentialUserEntity(options.user());
  mojo_options->challenge = ConvertBufferSource(options.challenge());

  Vector<webauth::mojom::blink::PublicKeyCredentialParametersPtr> parameters;
  for (const auto& parameter : options.parameters()) {
    parameters.push_back(ConvertPublicKeyCredentialParameters(parameter));
  }
  mojo_options->crypto_parameters = std::move(parameters);

  // Step 4 of https://w3c.github.io/webauthn/#createCredential
  WTF::TimeDelta adjusted_timeout;
  if (options.hasTimeout()) {
    adjusted_timeout = WTF::TimeDelta::FromMilliseconds(options.timeout());
    mojo_options->adjusted_timeout =
        std::max(kAdjustedTimeoutLower,
                 std::min(kAdjustedTimeoutUpper, adjusted_timeout));
  } else {
    adjusted_timeout = kAdjustedTimeoutLower;
  }

  if (options.hasExcludeList()) {
    // Adds the excludeList members
    for (const blink::PublicKeyCredentialDescriptor& descriptor :
         options.excludeList()) {
      auto mojo_descriptor =
          webauth::mojom::blink::PublicKeyCredentialDescriptor::New();
      mojo_descriptor->type = ConvertPublicKeyCredentialType(descriptor.type());
      mojo_descriptor->id = ConvertBufferSource(descriptor.id());
      for (const auto& transport : descriptor.transports())
        mojo_descriptor->transports.push_back(ConvertTransport(transport));
      mojo_options->exclude_credentials.push_back(std::move(mojo_descriptor));
    }
  }
  return mojo_options;
}

}  // namespace mojo

namespace blink {

WebAuthenticationClient::WebAuthenticationClient(LocalFrame& frame) {
  frame.GetInterfaceProvider().GetInterface(mojo::MakeRequest(&authenticator_));
  authenticator_.set_connection_error_handler(ConvertToBaseCallback(
      WTF::Bind(&WebAuthenticationClient::OnAuthenticatorConnectionError,
                WrapWeakPersistent(this))));
}

WebAuthenticationClient::~WebAuthenticationClient() {}

void WebAuthenticationClient::DispatchMakeCredential(
    const MakeCredentialOptions& publicKey,
    PublicKeyCallbacks* callbacks) {
  auto options = mojo::ConvertMakeCredentialOptions(publicKey);
  authenticator_->MakeCredential(
      std::move(options),
      ConvertToBaseCallback(
          WTF::Bind(&RespondToPublicKeyCallback,
                    WTF::Passed(new PublicKeyCallbacksWrapper(callbacks)))));
  /*
      WTF::Bind(&WebAuthenticationClient::OnMakeCredential,
                WTF::Passed(std::move(callbacks)))));
*/
  return;
}

void WebAuthenticationClient::GetAssertion(
    const PublicKeyCredentialRequestOptions& publicKey,
    PublicKeyCallbacks* callbacks) {
  NOTREACHED();
  return;
}

void WebAuthenticationClient::OnAuthenticatorConnectionError() {
  Cleanup();
}

DEFINE_TRACE(WebAuthenticationClient) {}

// Closes the Mojo connection.
void WebAuthenticationClient::Cleanup() {
  authenticator_.reset();
}

}  // namespace blink
