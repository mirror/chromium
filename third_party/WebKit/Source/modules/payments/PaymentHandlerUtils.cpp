// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentHandlerUtils.h"

#include "core/dom/ExecutionContext.h"
#include "core/inspector/ConsoleMessage.h"
#include "public/platform/modules/serviceworker/service_worker_error_type.mojom-shared.h"

using blink::mojom::ServiceWorkerResponseError;

namespace blink {

void PaymentHandlerUtils::ReportResponseError(
    ExecutionContext* execution_context,
    const String& event_name_prefix,
    ServiceWorkerResponseError error) {
  String error_message = event_name_prefix + ".respondWith() failed: ";
  switch (error) {
    case ServiceWorkerResponseError::PromiseRejected:
      error_message =
          error_message + "the promise passed to respondWith() was rejected.";
      break;
    case ServiceWorkerResponseError::DefaultPrevented:
      error_message =
          error_message +
          "preventDefault() was called without calling respondWith().";
      break;
    case ServiceWorkerResponseError::NoV8Instance:
      error_message = error_message +
                      "an object that was not a PaymentResponse was passed to "
                      "respondWith().";
      break;
    case ServiceWorkerResponseError::Unknown:
      error_message = error_message + "an unexpected error occurred.";
      break;
    case ServiceWorkerResponseError::ResponseTypeError:
    case ServiceWorkerResponseError::ResponseTypeOpaque:
    case ServiceWorkerResponseError::ResponseTypeNotBasicOrDefault:
    case ServiceWorkerResponseError::BodyUsed:
    case ServiceWorkerResponseError::ResponseTypeOpaqueForClientRequest:
    case ServiceWorkerResponseError::ResponseTypeOpaqueRedirect:
    case ServiceWorkerResponseError::BodyLocked:
    case ServiceWorkerResponseError::NoForeignFetchResponse:
    case ServiceWorkerResponseError::ForeignFetchHeadersWithoutOrigin:
    case ServiceWorkerResponseError::ForeignFetchMismatchedOrigin:
    case ServiceWorkerResponseError::RedirectedResponseForNotFollowRequest:
    case ServiceWorkerResponseError::DataPipeCreationFailed:
    // fallthrough
    case ServiceWorkerResponseError::kLast:
      NOTREACHED();
      error_message = error_message + "an unexpected error occurred.";
      break;
  }

  DCHECK(execution_context);
  execution_context->AddConsoleMessage(ConsoleMessage::Create(
      kJSMessageSource, kWarningMessageLevel, error_message));
}

}  // namespace blink
