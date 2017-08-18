// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebServiceWorkerErrorEnumTraits_h
#define WebServiceWorkerErrorEnumTraits_h

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerError.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/web_service_worker_error.mojom-shared.h"
namespace mojo {

template <>
struct EnumTraits<::blink::mojom::WebServiceWorkerErrorType,
                  ::blink::WebServiceWorkerError::ErrorType> {
  static ::blink::mojom::WebServiceWorkerErrorType ToMojom(
      ::blink::WebServiceWorkerError::ErrorType error) {
    switch (error) {
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNone:
        return ::blink::mojom::WebServiceWorkerErrorType::None;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeAbort:
        return ::blink::mojom::WebServiceWorkerErrorType::Abort;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeActivate:
        return ::blink::mojom::WebServiceWorkerErrorType::Activate;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeDisabled:
        return ::blink::mojom::WebServiceWorkerErrorType::Disabled;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeInstall:
        return ::blink::mojom::WebServiceWorkerErrorType::Install;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNavigation:
        return ::blink::mojom::WebServiceWorkerErrorType::Navigation;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNetwork:
        return ::blink::mojom::WebServiceWorkerErrorType::Network;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNotFound:
        return ::blink::mojom::WebServiceWorkerErrorType::NotFound;
      case ::blink::WebServiceWorkerError::ErrorType::
          kErrorTypeScriptEvaluateFailed:
        return ::blink::mojom::WebServiceWorkerErrorType::ScriptEvaluateFailed;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeSecurity:
        return ::blink::mojom::WebServiceWorkerErrorType::Security;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeState:
        return ::blink::mojom::WebServiceWorkerErrorType::State;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeTimeout:
        return ::blink::mojom::WebServiceWorkerErrorType::Timeout;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeUnknown:
        return ::blink::mojom::WebServiceWorkerErrorType::Unknown;
      case ::blink::WebServiceWorkerError::ErrorType::kErrorTypeType:
      // kErrorTypeType will never be passed via mojo.
      // fall through
      default:
        NOTREACHED();
        return ::blink::mojom::WebServiceWorkerErrorType::Unknown;
    }
  }

  static bool FromMojom(::blink::mojom::WebServiceWorkerErrorType error,
                        ::blink::WebServiceWorkerError::ErrorType* out) {
    switch (error) {
      case ::blink::mojom::WebServiceWorkerErrorType::None:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNone;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Abort:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeAbort;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Activate:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeActivate;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Disabled:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeDisabled;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Install:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeInstall;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Navigation:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNavigation;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Network:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNetwork;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::NotFound:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeNotFound;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::ScriptEvaluateFailed:
        *out = ::blink::WebServiceWorkerError::ErrorType::
            kErrorTypeScriptEvaluateFailed;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Security:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeSecurity;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::State:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeState;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Timeout:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeTimeout;
        return true;
      case ::blink::mojom::WebServiceWorkerErrorType::Unknown:
        *out = ::blink::WebServiceWorkerError::ErrorType::kErrorTypeUnknown;
        return true;
      default:
        NOTREACHED();
        return false;
    }
  }
};

}  // namespace mojo

#endif
