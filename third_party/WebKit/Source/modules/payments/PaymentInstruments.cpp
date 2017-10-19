// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentInstruments.h"

#include <utility>

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/modules/v8/V8BasicCardRequest.h"
#include "core/dom/DOMException.h"
#include "core/inspector/ConsoleMessage.h"
#include "modules/payments/BasicCardRequest.h"
#include "modules/payments/PaymentInstrument.h"
#include "modules/payments/PaymentManager.h"
#include "platform/wtf/Vector.h"
#include "public/platform/WebIconSizesParser.h"
#include "public/platform/modules/manifest/manifest.mojom-blink.h"

namespace blink {
namespace {

static const char kPaymentManagerUnavailable[] = "Payment manager unavailable";

static const size_t kMaxListSize = 1024;

bool rejectError(ScriptPromiseResolver* resolver,
                 payments::mojom::blink::PaymentHandlerStatus status) {
  switch (status) {
    case payments::mojom::blink::PaymentHandlerStatus::SUCCESS:
      return false;
    case payments::mojom::blink::PaymentHandlerStatus::NOT_IMPLEMENTED:
      resolver->Reject(
          DOMException::Create(kNotSupportedError, "Not implemented yet"));
      return true;
    case payments::mojom::blink::PaymentHandlerStatus::NOT_FOUND:
      resolver->Reject(DOMException::Create(kNotFoundError,
                                            "There is no stored instrument"));
      return true;
    case payments::mojom::blink::PaymentHandlerStatus::NO_ACTIVE_WORKER:
      resolver->Reject(
          DOMException::Create(kInvalidStateError, "No active service worker"));
      return true;
    case payments::mojom::blink::PaymentHandlerStatus::STORAGE_OPERATION_FAILED:
      resolver->Reject(DOMException::Create(kInvalidStateError,
                                            "Storage operation is failed"));
      return true;
    case payments::mojom::blink::PaymentHandlerStatus::
        FETCH_INSTRUMENT_ICON_FAILED:
      resolver->Reject(DOMException::Create(
          kNotFoundError, "Fetch or decode instrument icon failed"));
      return true;
    case payments::mojom::blink::PaymentHandlerStatus::
        FETCH_PAYMENT_APP_INFO_FAILED:
      // FETCH_PAYMENT_APP_INFO_FAILED indicates everything works well except
      // fetching payment handler's name and/or icon from its web app manifest.
      // The origin or name will be used to label this payment handler in
      // UI in this case, so only show warnning message instead of reject the
      // promise.
      ExecutionContext* context =
          ExecutionContext::From(resolver->GetScriptState());
      context->AddConsoleMessage(ConsoleMessage::Create(
          kJSMessageSource, kWarningMessageLevel,
          "Unable to fetch payment handler's name and/or icon from its web app "
          "manifest. User may not recognize this payment handler in UI, "
          "because it will be labeled only by its origin."));
      return false;
  }
  NOTREACHED();
  return false;
}

}  // namespace

PaymentInstruments::PaymentInstruments(
    const payments::mojom::blink::PaymentManagerPtr& manager)
    : manager_(manager) {}

ScriptPromise PaymentInstruments::deleteInstrument(
    ScriptState* script_state,
    const String& instrument_key) {
  if (!manager_.is_bound()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kPaymentManagerUnavailable));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  manager_->DeletePaymentInstrument(
      instrument_key, ConvertToBaseCallback(WTF::Bind(
                          &PaymentInstruments::onDeletePaymentInstrument,
                          WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

ScriptPromise PaymentInstruments::get(ScriptState* script_state,
                                      const String& instrument_key) {
  if (!manager_.is_bound()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kPaymentManagerUnavailable));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  manager_->GetPaymentInstrument(
      instrument_key, ConvertToBaseCallback(WTF::Bind(
                          &PaymentInstruments::onGetPaymentInstrument,
                          WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

ScriptPromise PaymentInstruments::keys(ScriptState* script_state) {
  if (!manager_.is_bound()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kPaymentManagerUnavailable));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  manager_->KeysOfPaymentInstruments(ConvertToBaseCallback(
      WTF::Bind(&PaymentInstruments::onKeysOfPaymentInstruments,
                WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

ScriptPromise PaymentInstruments::has(ScriptState* script_state,
                                      const String& instrument_key) {
  if (!manager_.is_bound()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kPaymentManagerUnavailable));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  manager_->HasPaymentInstrument(
      instrument_key, ConvertToBaseCallback(WTF::Bind(
                          &PaymentInstruments::onHasPaymentInstrument,
                          WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

ScriptPromise PaymentInstruments::set(ScriptState* script_state,
                                      const String& instrument_key,
                                      const PaymentInstrument& details,
                                      ExceptionState& exception_state) {
  if (!manager_.is_bound()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kPaymentManagerUnavailable));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  payments::mojom::blink::PaymentInstrumentPtr instrument =
      payments::mojom::blink::PaymentInstrument::New();
  instrument->name = details.hasName() ? details.name() : WTF::g_empty_string;
  if (details.hasIcons()) {
    ExecutionContext* context = ExecutionContext::From(script_state);
    for (const ImageObject image_object : details.icons()) {
      KURL parsed_url = context->CompleteURL(image_object.src());
      if (!parsed_url.IsValid() || !parsed_url.ProtocolIsInHTTPFamily()) {
        resolver->Reject(V8ThrowException::CreateTypeError(
            script_state->GetIsolate(),
            "'" + image_object.src() + "' is not a valid URL."));
        return promise;
      }

      mojom::blink::ManifestIconPtr icon = mojom::blink::ManifestIcon::New();
      icon->src = parsed_url;
      icon->type = image_object.type();
      icon->purpose.push_back(blink::mojom::ManifestIcon_Purpose::ANY);
      WebVector<WebSize> web_sizes =
          WebIconSizesParser::ParseIconSizes(image_object.sizes());
      for (const auto& web_size : web_sizes) {
        icon->sizes.push_back(web_size);
      }
      instrument->icons.push_back(std::move(icon));
    }
  }

  if (details.hasEnabledMethods()) {
    instrument->enabled_methods = details.enabledMethods();
  }

  if (details.hasCapabilities()) {
    v8::Local<v8::String> value;
    if (!v8::JSON::Stringify(script_state->GetContext(),
                             details.capabilities().V8Value().As<v8::Object>())
             .ToLocal(&value)) {
      exception_state.ThrowTypeError(
          "Capabilities should be a JSON-serializable object");
      return exception_state.Reject(script_state);
    }
    instrument->stringified_capabilities = ToCoreString(value);

    // Parse capabilities in render side to avoid parsing it in browser side.
    if (instrument->enabled_methods.Contains("basic-card")) {
      parseBasiccardCapabilities(details.capabilities(),
                                 instrument->supported_networks,
                                 instrument->supported_types, exception_state);
    }
  } else {
    instrument->stringified_capabilities = WTF::g_empty_string;
  }

  manager_->SetPaymentInstrument(
      instrument_key, std::move(instrument),
      ConvertToBaseCallback(
          WTF::Bind(&PaymentInstruments::onSetPaymentInstrument,
                    WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

void PaymentInstruments::parseBasiccardCapabilities(
    const ScriptValue& input,
    Vector<BasicCardNetwork>& supported_networks_output,
    Vector<BasicCardType>& supported_types_output,
    ExceptionState& exception_state) {
  DCHECK(!input.IsEmpty());

  BasicCardRequest basic_card;
  V8BasicCardRequest::ToImpl(input.GetIsolate(), input.V8Value(), basic_card,
                             exception_state);
  if (exception_state.HadException())
    return;

  if (basic_card.hasSupportedNetworks()) {
    if (basic_card.supportedNetworks().size() > kMaxListSize) {
      exception_state.ThrowTypeError(
          "basic-card supportedNetworks cannot be longer than 1024 elements");
      return;
    }

    const struct {
      const payments::mojom::BasicCardNetwork code;
      const char* const name;
    } kBasicCardNetworks[] = {{BasicCardNetwork::AMEX, "amex"},
                              {BasicCardNetwork::DINERS, "diners"},
                              {BasicCardNetwork::DISCOVER, "discover"},
                              {BasicCardNetwork::JCB, "jcb"},
                              {BasicCardNetwork::MASTERCARD, "mastercard"},
                              {BasicCardNetwork::MIR, "mir"},
                              {BasicCardNetwork::UNIONPAY, "unionpay"},
                              {BasicCardNetwork::VISA, "visa"}};

    for (const String& network : basic_card.supportedNetworks()) {
      for (size_t i = 0; i < arraysize(kBasicCardNetworks); ++i) {
        if (network == kBasicCardNetworks[i].name) {
          supported_networks_output.push_back(kBasicCardNetworks[i].code);
          break;
        }
      }
    }
  }

  if (basic_card.hasSupportedTypes()) {
    using ::payments::mojom::blink::BasicCardType;

    if (basic_card.supportedTypes().size() > kMaxListSize) {
      exception_state.ThrowTypeError(
          "basic-card supportedTypes cannot be longer than 1024 elements");
      return;
    }

    const struct {
      const BasicCardType code;
      const char* const name;
    } kBasicCardTypes[] = {{BasicCardType::CREDIT, "credit"},
                           {BasicCardType::DEBIT, "debit"},
                           {BasicCardType::PREPAID, "prepaid"}};

    for (const String& type : basic_card.supportedTypes()) {
      for (size_t i = 0; i < arraysize(kBasicCardTypes); ++i) {
        if (type == kBasicCardTypes[i].name) {
          supported_types_output.push_back(kBasicCardTypes[i].code);
          break;
        }
      }
    }
  }
}

ScriptPromise PaymentInstruments::clear(ScriptState* script_state) {
  if (!manager_.is_bound()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kPaymentManagerUnavailable));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  manager_->ClearPaymentInstruments(ConvertToBaseCallback(
      WTF::Bind(&PaymentInstruments::onClearPaymentInstruments,
                WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

DEFINE_TRACE(PaymentInstruments) {}

void PaymentInstruments::onDeletePaymentInstrument(
    ScriptPromiseResolver* resolver,
    payments::mojom::blink::PaymentHandlerStatus status) {
  DCHECK(resolver);
  resolver->Resolve(status ==
                    payments::mojom::blink::PaymentHandlerStatus::SUCCESS);
}

void PaymentInstruments::onGetPaymentInstrument(
    ScriptPromiseResolver* resolver,
    payments::mojom::blink::PaymentInstrumentPtr stored_instrument,
    payments::mojom::blink::PaymentHandlerStatus status) {
  DCHECK(resolver);
  if (rejectError(resolver, status))
    return;
  PaymentInstrument instrument;
  instrument.setName(stored_instrument->name);

  HeapVector<ImageObject> icons;
  for (const auto& icon : stored_instrument->icons) {
    ImageObject image_object;
    image_object.setSrc(icon->src.GetString());
    image_object.setType(icon->type);
    String sizes = WTF::g_empty_string;
    for (const auto& size : icon->sizes) {
      sizes = sizes + String::Format("%dx%d ", size.width, size.height);
    }
    image_object.setSizes(sizes.StripWhiteSpace());
    icons.emplace_back(image_object);
  }
  instrument.setIcons(icons);

  Vector<String> enabled_methods;
  for (const auto& method : stored_instrument->enabled_methods) {
    enabled_methods.push_back(method);
  }

  instrument.setEnabledMethods(enabled_methods);
  if (!stored_instrument->stringified_capabilities.IsEmpty()) {
    ScriptState::Scope scope(resolver->GetScriptState());
    ExceptionState exception_state(resolver->GetScriptState()->GetIsolate(),
                                   ExceptionState::kGetterContext,
                                   "PaymentInstruments", "get");
    instrument.setCapabilities(
        ScriptValue(resolver->GetScriptState(),
                    FromJSONString(resolver->GetScriptState()->GetIsolate(),
                                   stored_instrument->stringified_capabilities,
                                   exception_state)));
    if (exception_state.HadException()) {
      exception_state.Reject(resolver);
      return;
    }
  }
  resolver->Resolve(instrument);
}

void PaymentInstruments::onKeysOfPaymentInstruments(
    ScriptPromiseResolver* resolver,
    const Vector<String>& keys,
    payments::mojom::blink::PaymentHandlerStatus status) {
  DCHECK(resolver);
  if (rejectError(resolver, status))
    return;
  resolver->Resolve(keys);
}

void PaymentInstruments::onHasPaymentInstrument(
    ScriptPromiseResolver* resolver,
    payments::mojom::blink::PaymentHandlerStatus status) {
  DCHECK(resolver);
  resolver->Resolve(status ==
                    payments::mojom::blink::PaymentHandlerStatus::SUCCESS);
}

void PaymentInstruments::onSetPaymentInstrument(
    ScriptPromiseResolver* resolver,
    payments::mojom::blink::PaymentHandlerStatus status) {
  DCHECK(resolver);
  if (rejectError(resolver, status))
    return;
  resolver->Resolve();
}

void PaymentInstruments::onClearPaymentInstruments(
    ScriptPromiseResolver* resolver,
    payments::mojom::blink::PaymentHandlerStatus status) {
  DCHECK(resolver);
  if (rejectError(resolver, status))
    return;
  resolver->Resolve();
}

}  // namespace blink
