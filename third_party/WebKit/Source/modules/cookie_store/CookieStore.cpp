// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/cookie_store/CookieStore.h"

namespace blink {

CookieStore::CookieStore() {}

CookieStore* CookieStore::Create() {
  CookieStore* cookieStore = new CookieStore();
  return cookieStore;
}

ScriptPromise CookieStore::getAll(ScriptState* script_state,
                                  const String& name,
                                  const CookieStoreGetOptions& options,
                                  ExceptionState& exception_state) {
  return ScriptPromise::CastUndefined(script_state);
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
