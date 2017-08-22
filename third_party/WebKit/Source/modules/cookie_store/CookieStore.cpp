// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cookie_store/CookieStore.h"

#include <utility>
#include <vector>

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
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

namespace {

mojom::blink::CookieStoreGetOptionsPtr ToBackendOptions(
    const CookieStoreGetOptions& options) {
  auto backend_options = mojom::blink::CookieStoreGetOptions::New();
  backend_options->name = String("");
  backend_options->match_type = mojom::blink::CookieMatchType::STARTS_WITH;

  // TODO(crbug.com/729800): Implement, parse and validate options. Throw
  //                         relevant exceptions on validation errors.
  return backend_options;
}

mojom::blink::CookieStoreSetOptionsPtr ToBackendOptions(
    const KURL& cookie_url,
    const String& name,
    const String& value,
    const CookieStoreSetOptions& options) {
  auto backend_options = mojom::blink::CookieStoreSetOptions::New();

  DCHECK(!name.IsNull());
  backend_options->name = name;

  DCHECK(!value.IsNull());
  backend_options->value = value;

  // TODO(pwnall): What if expires isn't set?
  if (options.hasExpires())
    backend_options->expires = WTF::Time::FromJavaTime(options.expires());

  // TODO(pwnall): Find correct value.
  backend_options->domain = cookie_url.Host();
  backend_options->path = String("/");
  backend_options->secure = false;
  backend_options->http_only = false;

  // TODO(crbug.com/729800): Implement, parse and validate options. Throw
  //                         relevant exceptions on validation errors.
  return backend_options;
}

// Computes the cookie origin and site URLs for a ScriptState.
void ExtractCookieURLs(ScriptState* script_state,
                       KURL& cookie_url,
                       KURL& site_for_cookies) {
  ExecutionContext* execution_context = ExecutionContext::From(script_state);
  if (execution_context->IsDocument()) {
    Document* document = ToDocument(execution_context);
    cookie_url = document->CookieURL();
    site_for_cookies = document->SiteForCookies();
  } else {
    NOTREACHED();
  }
}

}  // anonymous namespace

CookieStore::~CookieStore() = default;

CookieStore* CookieStore::Create(ExecutionContext* execution_context,
                                 mojom::blink::CookieStorePtr backend) {
  return new CookieStore(execution_context, std::move(backend));
}

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  KURL cookie_url;
  KURL site_for_cookies;
  ExtractCookieURLs(script_state, cookie_url, site_for_cookies);

  mojom::blink::CookieStoreGetOptionsPtr backend_options =
      ToBackendOptions(options);
  DCHECK(!backend_options.is_null());

  if (!backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  backend_->GetAll(cookie_url, site_for_cookies, std::move(backend_options),
                   ConvertToBaseCallback(WTF::Bind(&CookieStore::OnGetAllResult,
                                                   WrapPersistent(this),
                                                   WrapPersistent(resolver))));
  return resolver->Promise();
}

ScriptPromise CookieStore::set(ScriptState* script_state,
                               const String& name,
                               const String& value,
                               const CookieStoreSetOptions& options,
                               ExceptionState& exception_state) {
  KURL cookie_url;
  KURL site_for_cookies;
  ExtractCookieURLs(script_state, cookie_url, site_for_cookies);

  mojom::blink::CookieStoreSetOptionsPtr backend_options =
      ToBackendOptions(cookie_url, name, value, options);
  DCHECK(!backend_options.is_null());

  if (!backend_) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "CookieStore backend went away");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  backend_->Set(cookie_url, site_for_cookies, std::move(backend_options),
                ConvertToBaseCallback(WTF::Bind(&CookieStore::OnSetResult,
                                                WrapPersistent(this),
                                                WrapPersistent(resolver))));
  return resolver->Promise();
}

void CookieStore::ContextDestroyed(ExecutionContext* execution_context) {
  backend_.reset();
}

CookieStore::CookieStore(ExecutionContext* execution_context,
                         mojom::blink::CookieStorePtr backend)
    : ContextLifecycleObserver(execution_context),
      backend_(std::move(backend)) {
  DCHECK(backend_);
}

void CookieStore::OnGetAllResult(
    ScriptPromiseResolver* resolver,
    Vector<mojom::blink::CookieInfoPtr> backend_cookies) {
  ScriptState* script_state = resolver->GetScriptState();
  if (!script_state->ContextIsValid())
    return;

  HeapVector<CookieListItem> cookies;
  cookies.ReserveCapacity(backend_cookies.size());
  for (const mojom::blink::CookieInfoPtr& backend_cookie : backend_cookies) {
    cookies.emplace_back();
    CookieListItem& cookie = cookies.back();
    cookie.setName(backend_cookie->name);
    cookie.setValue(backend_cookie->value);
  }

  resolver->Resolve(cookies);
}

void CookieStore::OnSetResult(ScriptPromiseResolver* resolver,
                              bool backend_success) {
  if (!backend_success) {
    resolver->Reject(DOMException::Create(
        kUnknownError, "An unknown error occured while writing the cookie."));
  }
  resolver->Resolve();
}

}  // namespace blink
