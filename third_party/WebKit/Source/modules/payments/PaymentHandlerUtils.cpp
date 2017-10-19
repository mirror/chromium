// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentHandlerUtils.h"

#include "core/dom/ExecutionContext.h"
#include "core/inspector/ConsoleMessage.h"

using blink::mojom::fetch::ResponseError;

namespace blink {

void PaymentHandlerUtils::ReportResponseError(
    ExecutionContext* execution_context,
    const String& event_name_prefix,
    ResponseError error) {
  String error_message = event_name_prefix + ".respondWith() failed: ";
  switch (error) {
    case ResponseError::PromiseRejected:
      error_message =
          error_message + "the promise passed to respondWith() was rejected.";
      break;
    case ResponseError::DefaultPrevented:
      error_message =
          error_message +
          "preventDefault() was called without calling respondWith().";
      break;
    case ResponseError::NoV8Instance:
      error_message = error_message +
                      "an object that was not a PaymentResponse was passed to "
                      "respondWith().";
      break;
    case ResponseError::Unknown:
      error_message = error_message + "an unexpected error occurred.";
      break;
    case ResponseError::ResponseTypeError:
    case ResponseError::ResponseTypeOpaque:
    case ResponseError::ResponseTypeNotBasicOrDefault:
    case ResponseError::BodyUsed:
    case ResponseError::ResponseTypeOpaqueForClientRequest:
    case ResponseError::ResponseTypeOpaqueRedirect:
    case ResponseError::BodyLocked:
    case ResponseError::NoForeignFetchResponse:
    case ResponseError::ForeignFetchHeadersWithoutOrigin:
    case ResponseError::ForeignFetchMismatchedOrigin:
    case ResponseError::RedirectedResponseForNotFollowRequest:
    case ResponseError::DataPipeCreationFailed:
    // fallthrough
    case ResponseError::kLast:
      NOTREACHED();
      error_message = error_message + "an unexpected error occurred.";
      break;
  }

  DCHECK(execution_context);
  execution_context->AddConsoleMessage(ConsoleMessage::Create(
      kJSMessageSource, kWarningMessageLevel, error_message));
}

}  // namespace blink
