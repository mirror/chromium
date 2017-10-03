// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/WebAuthenticationClient.h"

#include <utility>
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/LocalFrame.h"
#include "core/typed_arrays/DOMArrayBuffer.h"
#include "modules/credentialmanager/AuthenticatorAttestationResponse.h"
#include "modules/credentialmanager/MakeCredentialOptions.h"
#include "modules/credentialmanager/PublicKeyCredentialRequestOptions.h"
#include "platform/weborigin/OriginAccessEntry.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/platform/InterfaceProvider.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {
typedef ArrayBufferOrArrayBufferView BufferSource;

namespace {
using PublicKeyCallbacks = WebAuthenticationClient::PublicKeyCallbacks;

WebCredentialManagerError GetWebCredentialManagerErrorFromStatus(
    webauth::mojom::blink::AuthenticatorStatus status) {
  switch (status) {
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
    std::unique_ptr<PublicKeyCallbacks> callbacks,
    webauth::mojom::blink::AuthenticatorStatus status,
    webauth::mojom::blink::PublicKeyCredentialInfoPtr credential) {
  if (status != webauth::mojom::AuthenticatorStatus::SUCCESS) {
    DCHECK(!credential);
    callbacks->OnError(GetWebCredentialManagerErrorFromStatus(status));
    return;
  }

  DCHECK(credential);
  DCHECK(!credential->client_data_json.IsEmpty());
  callbacks->OnSuccess(std::move(credential));
}
}  // namespace

}  // namespace blink

namespace {
// Time to wait for an authenticator to successfully complete an operation.
constexpr WTF::TimeDelta kAdjustedTimeoutLower = WTF::TimeDelta::FromMinutes(1);
constexpr WTF::TimeDelta kAdjustedTimeoutUpper = WTF::TimeDelta::FromMinutes(2);
constexpr char kExtensionIdentifierAppId[] = "appid";
}  // namespace

namespace mojo {
using webauth::mojom::blink::AppIdExtension;
using webauth::mojom::blink::AppIdExtensionPtr;
using webauth::mojom::blink::AuthenticatorStatus;
using webauth::mojom::blink::MakeCredentialOptionsPtr;
using webauth::mojom::blink::PublicKeyCredentialEntityPtr;
using webauth::mojom::blink::PublicKeyCredentialParametersPtr;
using webauth::mojom::blink::PublicKeyCredentialRequestOptionsPtr;
using webauth::mojom::blink::PublicKeyCredentialType;
using webauth::mojom::blink::AuthenticatorTransport;

template <>
struct TypeConverter<Vector<uint8_t>, blink::BufferSource> {
  static Vector<uint8_t> Convert(const blink::BufferSource& buffer) {
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
};

template <>
struct TypeConverter<PublicKeyCredentialType, String> {
  static PublicKeyCredentialType Convert(const String& type) {
    if (type == "public-key")
      return PublicKeyCredentialType::PUBLIC_KEY;
    NOTREACHED();
    return PublicKeyCredentialType::PUBLIC_KEY;
  }
};

template <>
struct TypeConverter<AuthenticatorTransport, String> {
  static AuthenticatorTransport Convert(const String& transport) {
    if (transport == "usb")
      return AuthenticatorTransport::USB;
    if (transport == "nfc")
      return AuthenticatorTransport::NFC;
    if (transport == "ble")
      return AuthenticatorTransport::BLE;
    NOTREACHED();
    return AuthenticatorTransport::USB;
  }
};

template <>
struct TypeConverter<PublicKeyCredentialEntityPtr,
                     blink::PublicKeyCredentialUserEntity> {
  static PublicKeyCredentialEntityPtr Convert(
      const blink::PublicKeyCredentialUserEntity& user) {
    if (!(user.hasId() & user.hasName() & user.hasDisplayName())) {
      return nullptr;
    }
    auto entity = webauth::mojom::blink::PublicKeyCredentialEntity::New();
    entity->id = user.id();
    entity->name = user.name();
    if (user.hasIcon()) {
      entity->icon = blink::KURL(blink::KURL(), user.icon());
    }
    entity->display_name = user.displayName();
    return entity;
  }
};

template <>
struct TypeConverter<PublicKeyCredentialEntityPtr,
                     blink::PublicKeyCredentialEntity> {
  static PublicKeyCredentialEntityPtr Convert(
      const blink::PublicKeyCredentialEntity& rp) {
    if (!rp.hasName()) {
      return nullptr;
    }
    auto entity = webauth::mojom::blink::PublicKeyCredentialEntity::New();
    entity->id = rp.id();
    entity->name = rp.name();
    if (rp.hasIcon()) {
      entity->icon = blink::KURL(blink::KURL(), rp.icon());
    }
    return entity;
  }
};

template <>
struct TypeConverter<PublicKeyCredentialParametersPtr,
                     blink::PublicKeyCredentialParameters> {
  static PublicKeyCredentialParametersPtr Convert(
      const blink::PublicKeyCredentialParameters& parameter) {
    auto mojo_parameter =
        webauth::mojom::blink::PublicKeyCredentialParameters::New();
    mojo_parameter->type = ConvertTo<PublicKeyCredentialType>(parameter.type());
    if (parameter.algorithm() == "ES256") {
      mojo_parameter->algorithm_identifier = -7;
    }
    // A COSEAlgorithmIdentifier's value is a number identifying a cryptographic
    // algorithm. Values are registered in the IANA COSE Algorithms registry.
    // https://www.iana.org/assignments/cose/cose.xhtml#algorithms
    // mojo_parameter->algorithm_identifier = parameter.algorithm();
    return mojo_parameter;
  }
};

template <>
struct TypeConverter<MakeCredentialOptionsPtr, blink::MakeCredentialOptions> {
  static MakeCredentialOptionsPtr Convert(
      const blink::MakeCredentialOptions options) {
    auto mojo_options = webauth::mojom::blink::MakeCredentialOptions::New();
    mojo_options->relying_party =
        ConvertTo<PublicKeyCredentialEntityPtr>(options.rp());
    mojo_options->user =
        ConvertTo<PublicKeyCredentialEntityPtr>(options.user());
    if (!mojo_options->relying_party | !mojo_options->user) {
      return nullptr;
    }
    mojo_options->challenge = ConvertTo<Vector<uint8_t>>(options.challenge());

    // Step 4 of https://w3c.github.io/webauthn/#createCredential
    if (options.hasTimeout()) {
      WTF::TimeDelta adjusted_timeout;
      adjusted_timeout = WTF::TimeDelta::FromMilliseconds(options.timeout());
      mojo_options->adjusted_timeout =
          std::max(kAdjustedTimeoutLower,
                   std::min(kAdjustedTimeoutUpper, adjusted_timeout));
    } else {
      mojo_options->adjusted_timeout = kAdjustedTimeoutLower;
    }

    // Steps 8 and 9 of
    // https://www.w3.org/TR/2017/WD-webauthn-20170505/#createCredential
    Vector<PublicKeyCredentialParametersPtr> parameters;
    for (const auto& parameter : options.parameters()) {
      PublicKeyCredentialParametersPtr normalized_parameter =
          ConvertTo<PublicKeyCredentialParametersPtr>(parameter);
      if (normalized_parameter) {
        parameters.push_back(std::move(normalized_parameter));
      }
    }

    if (parameters.IsEmpty() && options.hasParameters()) {
      return nullptr;
    }

    mojo_options->crypto_parameters = std::move(parameters);

    if (options.hasExcludeList()) {
      // Adds the excludeList members
      for (const blink::PublicKeyCredentialDescriptor& descriptor :
           options.excludeList()) {
        auto mojo_descriptor =
            webauth::mojom::blink::PublicKeyCredentialDescriptor::New();
        mojo_descriptor->type =
            ConvertTo<PublicKeyCredentialType>(descriptor.type());
        mojo_descriptor->id = ConvertTo<Vector<uint8_t>>((descriptor.id()));
        if (descriptor.hasTransports()) {
          for (const auto& transport : descriptor.transports()) {
            mojo_descriptor->transports.push_back(
                ConvertTo<AuthenticatorTransport>(transport));
          }
        }
        mojo_options->exclude_credentials.push_back(std::move(mojo_descriptor));
      }
    }
    return mojo_options;
  }
};

template <>
struct TypeConverter<AppIdExtensionPtr, String> {
  static AppIdExtensionPtr Convert(const String app_id) {
    auto app_id_extension = webauth::mojom::blink::AppIdExtension::New();
    app_id_extension->fido_app_id = app_id;
    return app_id_extension;
  }
};

template <>
struct TypeConverter<PublicKeyCredentialRequestOptionsPtr,
                     blink::PublicKeyCredentialRequestOptions> {
  static PublicKeyCredentialRequestOptionsPtr Convert(
      const blink::PublicKeyCredentialRequestOptions options) {
    auto mojo_options =
        webauth::mojom::blink::PublicKeyCredentialRequestOptions::New();
    mojo_options->challenge = ConvertTo<Vector<uint8_t>>(options.challenge());

    if (options.hasTimeout()) {
      WTF::TimeDelta adjusted_timeout;
      adjusted_timeout = WTF::TimeDelta::FromMilliseconds(options.timeout());
      mojo_options->adjusted_timeout =
          std::max(kAdjustedTimeoutLower,
                   std::min(kAdjustedTimeoutUpper, adjusted_timeout));
    } else {
      mojo_options->adjusted_timeout = kAdjustedTimeoutLower;
    }

    mojo_options->relying_party_id = options.rpId();

    if (options.hasAllowList()) {
      // Adds the allowList members
      for (const blink::PublicKeyCredentialDescriptor& descriptor :
           options.allowList()) {
        auto mojo_descriptor =
            webauth::mojom::blink::PublicKeyCredentialDescriptor::New();
        mojo_descriptor->type =
            ConvertTo<PublicKeyCredentialType>(descriptor.type());
        mojo_descriptor->id = ConvertTo<Vector<uint8_t>>((descriptor.id()));
        if (descriptor.hasTransports()) {
          for (const auto& transport : descriptor.transports()) {
            mojo_descriptor->transports.push_back(
                ConvertTo<AuthenticatorTransport>(transport));
          }
        }
        mojo_options->allow_list.push_back(std::move(mojo_descriptor));
      }
    }

    // TODO refactor the extension stuff better (and supportive of more than one
    // type)
    if (options.hasExtensions()) {
      for (const std::pair<String, String>& extension : options.extensions()) {
        // Only the appId extension is currently supported.
        if (extension.first == kExtensionIdentifierAppId) {
          auto authenticator_extension =
              webauth::mojom::blink::AuthenticatorExtension::New();
          authenticator_extension->extension_identifier = extension.first;
          authenticator_extension->app_id_extension =
              ConvertTo<AppIdExtensionPtr>(extension.second);
          mojo_options->extensions.push_back(
              std::move(authenticator_extension));
        }
      }
    }
    return mojo_options;
  }
};

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
    std::unique_ptr<PublicKeyCallbacks> callbacks) {
  auto options =
      mojo::ConvertTo<webauth::mojom::blink::MakeCredentialOptionsPtr>(
          publicKey);
  if (!options) {
    callbacks->OnError(
        WebCredentialManagerError::kWebCredentialManagerNotSupportedError);
    return;
  }
  /*
    // Origin / domain checks.
    if (GetSecurityOrigin().IsUnique()) {
        callbacks->OnError(
            WebCredentialManagerError::kWebCredentialManagerNotAllowedError);
        return;
    }

    if (!options->relying_party->id) {
      options->relying_party->id = GetSecurityOrigin()->domain();
      // TODO how do I ensure this is valid?
    } else {
      OriginAccessEntry access_entry(options->relying_party->id,
          OriginAccessEntry::kAllowRegistrableDomains);
      if (access_entry.MatchesDomain(*GetSecurityOrigin()) ==
          OriginAccessEntry::kDoesNotMatchOrigin) {
        callbacks->OnError(
            WebCredentialManagerError::kWebCredentialManagerSecurityError);
        return;
      }
    } */

  authenticator_->MakeCredential(
      std::move(options),
      ConvertToBaseCallback(WTF::Bind(&RespondToPublicKeyCallback,
                                      WTF::Passed(std::move(callbacks)))));
  return;
}

void WebAuthenticationClient::DispatchGetAssertion(
    const PublicKeyCredentialRequestOptions& publicKey,
    std::unique_ptr<PublicKeyCallbacks> callbacks) {
  auto options = mojo::ConvertTo<
      webauth::mojom::blink::PublicKeyCredentialRequestOptionsPtr>(publicKey);
  authenticator_->GetAssertion(
      std::move(options),
      ConvertToBaseCallback(WTF::Bind(&RespondToPublicKeyCallback,
                                      WTF::Passed(std::move(callbacks)))));
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
