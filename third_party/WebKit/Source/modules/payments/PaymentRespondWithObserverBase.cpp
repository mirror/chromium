// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/payments/PaymentRespondWithObserverBase.h"

#include "base/logging.h"
#include "core/inspector/ConsoleMessage.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {
namespace {

// Returns the error message to let the developer know about the reason of the
// unusual failures.
const String GetMessageForResponseError(WebServiceWorkerResponseError error) {
  String error_message =
      "The respondWith() was rejected in PaymentRequestEvent: ";
  switch (error) {
    case kWebServiceWorkerResponseErrorPromiseRejected:
      error_message = error_message + "the promise was rejected.";
      break;
    case kWebServiceWorkerResponseErrorDefaultPrevented:
      error_message =
          error_message +
          "preventDefault() was called without calling respondWith().";
      break;
    case kWebServiceWorkerResponseErrorNoV8Instance:
      error_message = error_message +
                      "an object that was not a PaymentResponse was passed to "
                      "respondWith().";
      break;
    case kWebServiceWorkerResponseErrorResponseTypeError:
      error_message = error_message +
                      "the promise was resolved with an error response object.";
      break;
    case kWebServiceWorkerResponseErrorUnknown:
      error_message = error_message + "an unexpected error occurred.";
      break;
    case kWebServiceWorkerResponseErrorResponseTypeOpaque:
    case kWebServiceWorkerResponseErrorResponseTypeNotBasicOrDefault:
    case kWebServiceWorkerResponseErrorBodyUsed:
    case kWebServiceWorkerResponseErrorResponseTypeOpaqueForClientRequest:
    case kWebServiceWorkerResponseErrorResponseTypeOpaqueRedirect:
    case kWebServiceWorkerResponseErrorBodyLocked:
    case kWebServiceWorkerResponseErrorNoForeignFetchResponse:
    case kWebServiceWorkerResponseErrorForeignFetchHeadersWithoutOrigin:
    case kWebServiceWorkerResponseErrorForeignFetchMismatchedOrigin:
    case kWebServiceWorkerResponseErrorRedirectedResponseForNotFollowRequest:
      NOTREACHED();
      error_message = error_message + "an unexpected error occurred.";
      break;
  }
  return error_message;
}

}  // namespace

void PaymentRespondWithObserverBase::OnResponseRejected(
    WebServiceWorkerResponseError error) {
  DCHECK(GetExecutionContext());
  GetExecutionContext()->AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kWarningMessageLevel,
                             GetMessageForResponseError(error)));
}

PaymentRespondWithObserverBase::PaymentRespondWithObserverBase(
    ExecutionContext* context,
    int event_id,
    WaitUntilObserver* observer)
    : RespondWithObserver(context, event_id, observer) {}

}  // namespace blink
