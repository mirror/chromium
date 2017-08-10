// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cookie_store/CookieStore.h"

#include <utility>
#include <vector>

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "modules/cookie_store/CookieListItem.h"
#include "modules/cookie_store/CookieStoreGetOptions.h"
#include "modules/cookie_store/CookieStoreSetOptions.h"
#include "modules/serviceworkers/ServiceWorkerGlobalScope.h"
#include "platform/bindings/ScriptState.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/Functional.h"
#include "platform/wtf/Optional.h"
#include "platform/wtf/Time.h"
#include "public/platform/modules/cookie_store/cookie_store.mojom-blink.h"

namespace blink {

CookieStore::CookieStore(mojom::blink::CookieStorePtr backend)
    : backend_(std::move(backend)) {
  DCHECK(backend_);
  backend_.set_connection_error_handler(ConvertToBaseCallback(WTF::Bind(
      &CookieStore::OnBackendConnectionError, WrapWeakPersistent(this))));
}

CookieStore::~CookieStore() = default;

CookieStore* CookieStore::Create(mojom::blink::CookieStorePtr backend) {
  CookieStore* cookieStore = new CookieStore(std::move(backend));
  return cookieStore;
}

void CookieStore::OnBackendConnectionError() {
  backend_.reset();
}

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const String& name,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  KURL cookie_url;
  KURL first_party_for_cookies;
  ExtractCookieURLs(script_state, &cookie_url, &first_party_for_cookies);

  mojom::blink::CookieStoreGetOptionsPtr backend_options =
      ToBackendOptions(options, exception_state);
  if (backend_options.is_null())
    return ScriptPromise();

  if (!backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  backend_->GetAll(cookie_url, first_party_for_cookies, name,
                   std::move(backend_options),
                   ConvertToBaseCallback(WTF::Bind(&CookieStore::OnGetAllResult,
                                                   WrapPersistent(this),
                                                   WrapPersistent(resolver))));
  return resolver->Promise();
}

void CookieStore::ExtractCookieURLs(ScriptState* script_state,
                                    KURL* cookie_url,
                                    KURL* first_party_for_cookies) {
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  if (execution_context->IsDocument()) {
    Document* document = ToDocument(execution_context);
    *cookie_url = document->CookieURL();
    *first_party_for_cookies = document->FirstPartyForCookies();
  } else if (execution_context->IsServiceWorkerGlobalScope()) {
    // TODO(pwnall): What are the correct URL values here?
    *cookie_url = execution_context->ContextURL();
    *first_party_for_cookies = execution_context->ContextURL();
  }
}

void CookieStore::OnGetAllResult(
    ScriptPromiseResolver* resolver,
    WTF::Vector<mojom::blink::CookieInfoPtr> backend_result) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  // V8 scope needed because creating a dictionary involves a constructor call.
  ScriptState::Scope scope(script_state);

  HeapVector<CookieListItem> result;
  result.ReserveCapacity(backend_result.size());
  for (const mojom::blink::CookieInfoPtr& backend_cookie : backend_result) {
    result.emplace_back();
    CookieListItem& cookie = result.back();
    cookie.setName(backend_cookie->name);
    cookie.setValue(backend_cookie->value);
  }

  resolver->Resolve(result);
}

mojom::blink::CookieStoreGetOptionsPtr CookieStore::ToBackendOptions(
    const CookieStoreGetOptions& options,
    ExceptionState& exception_state) {
  mojom::blink::CookieStoreGetOptionsPtr backend_options(WTF::in_place);
  if (options.hasURL()) {
    KURL url = KURL(kParsedURLString, options.url());
    if (!url.IsValid()) {
      exception_state.ThrowTypeError("Invalid URL");
      return nullptr;
    }
    backend_options->url = std::move(url);
  }

  if (options.hasName())
    backend_options->name = options.name();
  backend_options->match_type = mojom::blink::CookieMatchType::EQUALS;
  if (options.matchType() == "startsWith")
    backend_options->match_type = mojom::blink::CookieMatchType::STARTS_WITH;

  return backend_options;
}

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  // TODO(pwnall): Implement in terms of getAll above.
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::get(ScriptState* script_state,
                               const String& name,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  // TODO(pwnall): Implement in terms of getAll.
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::get(ScriptState* script_state,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  // TODO(pwnall): Implement in terms of getAll.
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::has(ScriptState* script_state,
                               const String& name,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  // TODO(pwnall): Implement in terms of getAll.
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::has(ScriptState* script_state,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  // TODO(pwnall): Implement in terms of getAll.
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::set(ScriptState* script_state,
                               const String& name,
                               const String& value,
                               const CookieStoreSetOptions& options,
                               ExceptionState& exception_state) {
  KURL cookie_url;
  KURL first_party_for_cookies;
  ExtractCookieURLs(script_state, &cookie_url, &first_party_for_cookies);

  mojom::blink::CookieStoreSetOptionsPtr backend_options =
      ToBackendOptions(cookie_url, options, exception_state);
  if (backend_options.is_null())
    return ScriptPromise();

  if (!backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  backend_->Set(cookie_url, first_party_for_cookies, name, value,
                std::move(backend_options),
                ConvertToBaseCallback(WTF::Bind(&CookieStore::OnSetResult,
                                                WrapPersistent(this),
                                                WrapPersistent(resolver))));
  return resolver->Promise();
}

void CookieStore::OnSetResult(ScriptPromiseResolver* resolver,
                              bool backend_result) {
  (void)(backend_result);
  resolver->Resolve();
}

mojom::blink::CookieStoreSetOptionsPtr CookieStore::ToBackendOptions(
    const KURL& cookie_url,
    const CookieStoreSetOptions& options,
    ExceptionState& exception_state) {
  mojom::blink::CookieStoreSetOptionsPtr backend_options(WTF::in_place);
  // TODO(pwnall): What if expires isn't set?
  if (options.hasExpires())
    backend_options->expires = WTF::Time::FromJavaTime(options.expires());

  // TODO(pwnall): Checks and exception throwing.
  if (options.hasDomain()) {
    backend_options->domain = options.domain();
  } else {
    // TODO(pwnall): Correct value?
    backend_options->domain = cookie_url.Host();
  }

  backend_options->path = options.path();

  // TODO(pwnall): Checks and exception throwing.
  backend_options->secure = options.secure();
  backend_options->http_only = options.httpOnly();

  return backend_options;
}

ScriptPromise CookieStore::deleteFunction(ScriptState* script_state,
                                          const String& name,
                                          const CookieStoreSetOptions& options,
                                          ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

}  // namespace blink
