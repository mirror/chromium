// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/background_fetch/BackgroundFetchManager.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "bindings/modules/v8/RequestOrUSVString.h"
#include "bindings/modules/v8/RequestOrUSVStringOrRequestOrUSVStringSequence.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/Deprecation.h"
#include "core/frame/UseCounter.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "core/loader/MixedContentChecker.h"
#include "modules/background_fetch/BackgroundFetchBridge.h"
#include "modules/background_fetch/BackgroundFetchOptions.h"
#include "modules/background_fetch/BackgroundFetchRegistration.h"
#include "modules/fetch/Request.h"
#include "modules/serviceworkers/ServiceWorkerRegistration.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/V8ThrowException.h"
#include "platform/loader/fetch/FetchUtils.h"
#include "platform/network/NetworkUtils.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/KnownPorts.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerRequest.h"

namespace blink {

namespace {

// Message for the TypeError thrown when an empty request sequence is seen.
const char kEmptyRequestSequenceErrorMessage[] =
    "At least one request must be given.";

// Message for the TypeError thrown when a null request is seen.
const char kNullRequestErrorMessage[] = "Requests must not be null.";

ScriptPromise RejectWithTypeError(ScriptState* script_state,
                                  const KURL& request_url,
                                  const String& reason) {
  return ScriptPromise::Reject(
      script_state, V8ThrowException::CreateTypeError(
                        script_state->GetIsolate(),
                        "Refused to connect to '" + request_url.ElidedString() +
                            "' because " + reason + "."));
}

bool ShouldBlockDueToCSP(ExecutionContext* execution_context,
                         const KURL& request_url) {
  return !ContentSecurityPolicy::ShouldBypassMainWorld(execution_context) &&
         !execution_context->GetContentSecurityPolicy()->AllowConnectToSource(
             request_url);
}

bool ShouldBlockPort(const KURL& request_url) {
  // https://fetch.spec.whatwg.org/#block-bad-port
  return !IsPortAllowedForScheme(request_url);
}

bool ShouldBlockCredentials(ExecutionContext* execution_context,
                            const KURL& request_url) {
  // Allow empty credentials.
  if (request_url.User().IsEmpty() && request_url.Pass().IsEmpty())
    return false;
  // Allow equal credentials.
  // TODO(crbug.com/711354): Should we use the Service Worker's script url here
  // instead, when the ExecutionContext is a document?
  if (execution_context->Url().User() == request_url.User() &&
      execution_context->Url().Pass() == request_url.Pass()) {
    return false;
  }

  Deprecation::CountDeprecation(
      execution_context,
      WebFeature::kRequestedSubresourceWithEmbeddedCredentials);

  // TODO(mkwst): Remove the runtime check one way or the other once we're
  // sure it's going to stick (or that it's not).
  return RuntimeEnabledFeatures::BlockCredentialedSubresourcesEnabled();
}

bool ShouldBlockScheme(const KURL& request_url) {
  // Require http(s), i.e. block data:, wss: and file:
  // https://github.com/WICG/background-fetch/issues/44
  return !request_url.ProtocolIs(WTF::g_http_atom) &&
         !request_url.ProtocolIs(WTF::g_https_atom);
}

bool ShouldBlockMixedContent(ExecutionContext* execution_context,
                             const KURL& request_url) {
  // TODO(crbug.com/711354): Using MixedContentChecker::ShouldBlockFetch would
  // log better metrics.
  if (MixedContentChecker::IsMixedContent(
          execution_context->GetSecurityOrigin(), request_url)) {
    return true;
  }

  // Normally requests from e.g. http://127.0.0.1 aren't subject to Mixed
  // Content checks even though that is a secure context. Since this is a new
  // API only exposed on secure contexts, be strict pending the discussion in
  // https://groups.google.com/a/chromium.org/d/topic/security-dev/29Ftfgn-w0I/discussion
  // https://w3c.github.io/webappsec-mixed-content/#a-priori-authenticated-url
  if (!SecurityOrigin::Create(request_url)->IsPotentiallyTrustworthy() &&
      !request_url.ProtocolIsData()) {
    return true;
  }

  return false;
}

bool ShouldBlockDanglingMarkup(ExecutionContext* execution_context,
                               const KURL& request_url) {
  if (!request_url.PotentiallyDanglingMarkup() ||
      !request_url.ProtocolIsInHTTPFamily()) {
    return false;
  }

  Deprecation::CountDeprecation(
      execution_context, WebFeature::kCanRequestURLHTTPContainingNewline);

  return RuntimeEnabledFeatures::RestrictCanRequestURLCharacterSetEnabled();
}

bool ShouldBlockCORSPreflight(ExecutionContext* execution_context,
                              const WebServiceWorkerRequest& web_request,
                              const KURL& request_url) {
  // Requests that require CORS preflights are temporarily blocked, because the
  // browser side of Background Fetch doesn't yet support performing CORS
  // checks. TODO(crbug.com/711354): Remove this temporary block.

  // Same origin requests don't require a CORS preflight.
  // https://fetch.spec.whatwg.org/#main-fetch
  // TODO(crbug.com/711354): Make sure that cross-origin redirects are disabled.
  bool same_origin =
      execution_context->GetSecurityOrigin()->CanRequestNoSuborigin(
          request_url);
  if (same_origin)
    return false;

  // Requests that are more involved than what is possible with HTML's form
  // element require a CORS-preflight request.
  // https://fetch.spec.whatwg.org/#main-fetch
  if (!FetchUtils::IsCORSSafelistedMethod(web_request.Method()) ||
      !FetchUtils::ContainsOnlyCORSSafelistedHeaders(web_request.Headers())) {
    return true;
  }

  if (RuntimeEnabledFeatures::CorsRFC1918Enabled()) {
    WebAddressSpace requestor_space =
        execution_context->GetSecurityContext().AddressSpace();

    // TODO(mkwst): This only checks explicit IP addresses. We'll have to move
    // all this up to //net and //content in order to have any real impact on
    // gateway attacks. That turns out to be a TON of work (crbug.com/378566).
    WebAddressSpace target_space = kWebAddressSpacePublic;
    if (NetworkUtils::IsReservedIPAddress(request_url.Host()))
      target_space = kWebAddressSpacePrivate;
    if (SecurityOrigin::Create(request_url)->IsLocalhost())
      target_space = kWebAddressSpaceLocal;

    bool is_external_request = requestor_space > target_space;
    if (is_external_request)
      return true;
  }

  return false;
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

  ExecutionContext* execution_context = ExecutionContext::From(script_state);

  // Based on security steps from https://fetch.spec.whatwg.org/#main-fetch
  // TODO(crbug.com/711354): Remove all this duplicative code once Fetch (and
  // all its security checks) are implemented in the Network Service, such that
  // the Download Service in the browser process can use it without having to
  // spin up a renderer process.
  for (const WebServiceWorkerRequest& web_request : web_requests) {
    // TODO(crbug.com/711354): Decide whether to support upgrading requests to
    // potentially secure URLs (https://w3c.github.io/webappsec-upgrade-
    // insecure-requests/) and/or HSTS rewriting. Since this is a new API only
    // exposed on Secure Contexts, and the Mixed Content check below will block
    // any requests to insecure contexts, it'd be cleanest not to support it.
    // Depends how closely compatible with Fetch we want to be. If support is
    // added, make sure to report CSP violations before upgrading the URL.

    KURL request_url(web_request.Url());

    if (!request_url.IsValid())
      return RejectWithTypeError(script_state, request_url, "it is invalid");

    // Check this before mixed content, so that if mixed content is blocked by
    // CSP they get a CSP warning rather than a mixed content warning.
    if (ShouldBlockDueToCSP(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "it violates the document's Content Security "
                                 "Policy");
    }

    if (ShouldBlockPort(request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "that port is not allowed");
    }

    if (ShouldBlockCredentials(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "credentials are not allowed in subresource "
                                 "requests");
    }

    if (ShouldBlockScheme(request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "only the https: scheme is allowed (or http: "
                                 "for loopback IPs)");
    }

    // Check for mixed content. We do this after CSP so that when folks block
    // mixed content via CSP, they don't get a mixed content warning, but a CSP
    // warning instead.
    if (ShouldBlockMixedContent(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "it is insecure; use https instead");
    }

    if (ShouldBlockDanglingMarkup(execution_context, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "it contains dangling markup)");
    }

    if (ShouldBlockCORSPreflight(execution_context, web_request, request_url)) {
      return RejectWithTypeError(script_state, request_url,
                                 "CORS preflights are not yet supported "
                                 "by this browser");
    }
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
