// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/background_fetch/BackgroundFetchManager.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/modules/v8/RequestOrUSVString.h"
#include "bindings/modules/v8/RequestOrUSVStringOrRequestOrUSVStringSequence.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "modules/background_fetch/BackgroundFetchBridge.h"
#include "modules/background_fetch/BackgroundFetchOptions.h"
#include "modules/background_fetch/BackgroundFetchRegistration.h"
#include "modules/fetch/Request.h"
#include "modules/serviceworkers/ServiceWorkerRegistration.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/V8ThrowException.h"
#include "platform/loader/fetch/FetchUtils.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerRequest.h"

namespace blink {

namespace {

// Message for the TypeError thrown when an empty request sequence is seen.
const char kEmptyRequestSequenceErrorMessage[] =
    "At least one request must be given.";

// Message for the TypeError thrown when a null request is seen.
const char kNullRequestErrorMessage[] = "Requests must not be null.";

bool AllHttpsOrLocalhost(const Vector<WebServiceWorkerRequest>& web_requests) {
  for (const WebServiceWorkerRequest& web_request : web_requests) {
    KURL url(web_request.Url());
    if (!SecurityOrigin::Create(url)->IsPotentiallyTrustworthy())
      return false;
    // Additionally disallow schemes like wss: and file: by requiring http(s):.
    // The previous check ensures that http: is only localhost.
    if (!url.ProtocolIs(WTF::g_https_atom) && !url.ProtocolIs(WTF::g_http_atom))
      return false;
  }
  return true;
}

bool AllSameOriginOrCORSSafelisted(
    ScriptState* script_state,
    const Vector<WebServiceWorkerRequest>& web_requests) {
  SecurityOrigin* origin =
      ExecutionContext::From(script_state)->GetSecurityOrigin();
  for (const WebServiceWorkerRequest& web_request : web_requests) {
    bool same_origin = origin->CanRequestNoSuborigin(web_request.Url());
    // TODO: Should this additionally check the request mode?
    // TODO: Make sure that cross origin redirects are disabled.
    if (same_origin)
      continue;
    if (!FetchUtils::IsCORSSafelistedMethod(web_request.Method()) ||
        !FetchUtils::ContainsOnlyCORSSafelistedHeaders(web_request.Headers())) {
      return false;
    }
  }
  return true;
}

}  // namespace

BackgroundFetchManager::BackgroundFetchManager(
    ServiceWorkerRegistration* registration)
    : registration_(registration) {
  DCHECK(registration);
  bridge_ = BackgroundFetchBridge::From(registration_);
}

ScriptPromise BackgroundFetchManager::fetch(
    ScriptState* script_state,
    const String& tag,
    const RequestOrUSVStringOrRequestOrUSVStringSequence& requests,
    const BackgroundFetchOptions& options,
    ExceptionState& exception_state) {
  if (!registration_->active()) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "No active registration available on "
                                          "the ServiceWorkerRegistration."));
  }

  Vector<WebServiceWorkerRequest> web_requests =
      CreateWebRequestVector(script_state, requests, exception_state);
  if (exception_state.HadException())
    return ScriptPromise();

  if (!AllHttpsOrLocalhost(web_requests)) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "Requests must use the https: scheme "
                                          "(or http: only if the origin is "
                                          "localhost)."));
  }

  if (!AllSameOriginOrCORSSafelisted(script_state, web_requests)) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "Requests requiring a CORS preflight "
                                          "are not yet supported by this "
                                          "browser."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  bridge_->Fetch(tag, std::move(web_requests), options,
                 WTF::Bind(&BackgroundFetchManager::DidFetch,
                           WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

void BackgroundFetchManager::DidFetch(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error,
    BackgroundFetchRegistration* registration) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
      DCHECK(registration);
      resolver->Resolve(registration);
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_TAG:
      DCHECK(!registration);
      resolver->Reject(DOMException::Create(
          kInvalidStateError,
          "There already is a registration for the given tag."));
      return;
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
    case mojom::blink::BackgroundFetchError::INVALID_TAG:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

ScriptPromise BackgroundFetchManager::get(ScriptState* script_state,
                                          const String& tag) {
  if (!registration_->active()) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "No active registration available on "
                                          "the ServiceWorkerRegistration."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  bridge_->GetRegistration(
      tag, WTF::Bind(&BackgroundFetchManager::DidGetRegistration,
                     WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

// static
Vector<WebServiceWorkerRequest> BackgroundFetchManager::CreateWebRequestVector(
    ScriptState* script_state,
    const RequestOrUSVStringOrRequestOrUSVStringSequence& requests,
    ExceptionState& exception_state) {
  Vector<WebServiceWorkerRequest> web_requests;

  if (requests.isRequestOrUSVStringSequence()) {
    HeapVector<RequestOrUSVString> request_vector =
        requests.getAsRequestOrUSVStringSequence();

    // Throw a TypeError when the developer has passed an empty sequence.
    if (!request_vector.size()) {
      exception_state.ThrowTypeError(kEmptyRequestSequenceErrorMessage);
      return Vector<WebServiceWorkerRequest>();
    }

    web_requests.resize(request_vector.size());

    for (size_t i = 0; i < request_vector.size(); ++i) {
      const RequestOrUSVString& request_or_url = request_vector[i];

      Request* request = nullptr;
      if (request_or_url.isRequest()) {
        request = request_or_url.getAsRequest();
      } else if (request_or_url.isUSVString()) {
        request = Request::Create(script_state, request_or_url.getAsUSVString(),
                                  exception_state);
        if (exception_state.HadException())
          return Vector<WebServiceWorkerRequest>();
      } else {
        exception_state.ThrowTypeError(kNullRequestErrorMessage);
        return Vector<WebServiceWorkerRequest>();
      }

      DCHECK(request);
      request->PopulateWebServiceWorkerRequest(web_requests[i]);
    }
  } else if (requests.isRequest()) {
    DCHECK(requests.getAsRequest());
    web_requests.resize(1);
    requests.getAsRequest()->PopulateWebServiceWorkerRequest(web_requests[0]);
  } else if (requests.isUSVString()) {
    Request* request = Request::Create(script_state, requests.getAsUSVString(),
                                       exception_state);
    if (exception_state.HadException())
      return Vector<WebServiceWorkerRequest>();

    DCHECK(request);
    web_requests.resize(1);
    request->PopulateWebServiceWorkerRequest(web_requests[0]);
  } else {
    exception_state.ThrowTypeError(kNullRequestErrorMessage);
    return Vector<WebServiceWorkerRequest>();
  }

  return web_requests;
}

void BackgroundFetchManager::DidGetRegistration(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error,
    BackgroundFetchRegistration* registration) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
    case mojom::blink::BackgroundFetchError::INVALID_TAG:
      resolver->Resolve(registration);
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_TAG:
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

ScriptPromise BackgroundFetchManager::getTags(ScriptState* script_state) {
  if (!registration_->active()) {
    return ScriptPromise::Reject(
        script_state,
        V8ThrowException::CreateTypeError(script_state->GetIsolate(),
                                          "No active registration available on "
                                          "the ServiceWorkerRegistration."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  bridge_->GetTags(WTF::Bind(&BackgroundFetchManager::DidGetTags,
                             WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

void BackgroundFetchManager::DidGetTags(
    ScriptPromiseResolver* resolver,
    mojom::blink::BackgroundFetchError error,
    const Vector<String>& tags) {
  switch (error) {
    case mojom::blink::BackgroundFetchError::NONE:
      resolver->Resolve(tags);
      return;
    case mojom::blink::BackgroundFetchError::DUPLICATED_TAG:
    case mojom::blink::BackgroundFetchError::INVALID_ARGUMENT:
    case mojom::blink::BackgroundFetchError::INVALID_TAG:
      // Not applicable for this callback.
      break;
  }

  NOTREACHED();
}

DEFINE_TRACE(BackgroundFetchManager) {
  visitor->Trace(registration_);
  visitor->Trace(bridge_);
}

}  // namespace blink
