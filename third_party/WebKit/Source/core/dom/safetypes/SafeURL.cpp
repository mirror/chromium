// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/safetypes/SafeURL.h"

#include "core/dom/ExecutionContext.h"
#include "platform/bindings/ScriptState.h"

namespace blink {

SafeURL::SafeURL(const KURL& url) : url_(url) {}

SafeURL* SafeURL::create(ScriptState* script_state, const String& url) {
  KURL result(ExecutionContext::From(script_state)->CompleteURL(url));

  if (!result.IsValid() || !result.ProtocolIsInHTTPFamily())
    result = KURL(kParsedURLString, "about:invalid");

  return SafeURL::Create(result);
}

SafeURL* SafeURL::unsafelyCreate(ScriptState* script_state, const String& url) {
  return SafeURL::Create(ExecutionContext::From(script_state)->CompleteURL(url));
}

String SafeURL::toString() const {
  return url_.GetString();
}

}  // namespace blink
