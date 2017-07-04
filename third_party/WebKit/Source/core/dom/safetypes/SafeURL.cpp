// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/safetypes/SafeURL.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/ExecutionContext.h"
#include "platform/weborigin/KURL.h"

namespace blink {

SafeURL::SafeURL(const KURL& url) : url_(url) {}

ScriptPromise SafeURL::create(ScriptState* script_state, const String& url) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  KURL result(ExecutionContext::From(script_state)->Url(), url);

  if (!result.IsValid() || !result.ProtocolIsInHTTPFamily())
    result = KURL(kParsedURLString, "about:invalid");

  resolver->Resolve(SafeURL::Create(result));
  return promise;
}

ScriptPromise SafeURL::unsafelyCreate(ScriptState* script_state,
                                      const String& url) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  resolver->Resolve(
      SafeURL::Create(KURL(ExecutionContext::From(script_state)->Url(), url)));
  return promise;
}

String SafeURL::toString() const {
  return url_.GetString();
}

}  // namespace blink
