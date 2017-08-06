// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cookie_store/CookieStore.h"

#include <utility>
#include <vector>

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "platform/bindings/ScriptState.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.cpp"
#include "platform/wtf/Functional.h"
#include "platform/wtf/Optional.h"
#include "public/platform/modules/cookie_store/cookie_store.mojom-blink.h"

namespace blink {

CookieStore::CookieStore(mojom::blink::CookieStorePtr backend)
    : backend_(std::move(backend)) {}
CookieStore::~CookieStore() = default;

CookieStore* CookieStore::Create(mojom::blink::CookieStorePtr backend) {
  CookieStore* cookieStore = new CookieStore(std::move(backend));
  return cookieStore;
}

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const String& name,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  mojom::blink::CookieStoreGetOptionsPtr backend_options =
      ToBackendOptions(options, exception_state);
  if (!backend_options.is_null())
    return ScriptPromise();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  KURL derp_url(kParsedURLString, "https://www.google.com");
  backend_->GetAll(derp_url, derp_url, name, std::move(backend_options),
                   ConvertToBaseCallback(WTF::Bind(&CookieStore::OnGetAllResult,
                                                   WrapPersistent(this),
                                                   WrapPersistent(resolver))));

  return resolver->Promise();
}

void CookieStore::OnGetAllResult(
    ScriptPromiseResolver* resolver,
    WTF::Vector<mojom::blink::CookieInfoPtr> result) {
  ScriptState* script_state = resolver->GetScriptState();
  resolver->Resolve(v8::True(script_state->GetIsolate()));
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
                                  const CookieStoreGetOptions& more_options,
                                  ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::get(ScriptState* script_state,
                               const String& name,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::get(ScriptState* script_state,
                               const CookieStoreGetOptions& options,
                               const CookieStoreGetOptions& more_options,
                               ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::has(ScriptState* script_state,
                               const String& name,
                               const CookieStoreGetOptions& options,
                               ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::has(ScriptState* script_state,
                               const CookieStoreGetOptions& options,
                               const CookieStoreGetOptions& more_options,
                               ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::set(ScriptState* script_state,
                               const String& name,
                               const String& value,
                               const CookieStoreSetOptions& options,
                               ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

ScriptPromise CookieStore::remove(ScriptState* script_state,
                                  const String& name,
                                  const CookieStoreSetOptions& options,
                                  ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
}

}  // namespace blink
