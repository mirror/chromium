// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/safetypes/SafeHTML.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"

namespace blink {

SafeHTML::SafeHTML(const String& html) : html_(html) {}

ScriptPromise SafeHTML::escape(ScriptState* script_state, const String& html) {
  // TODO(mkwst): This could be hugely optimized by scanning the string for any
  // of the interesting characters to see whether we need to do any replacement
  // at all, and by replacing all the characters in a single pass.
  String escapedHTML(html);
  escapedHTML.Replace("&", "&amp;")
      .Replace("<", "&lt;")
      .Replace(">", "&gt;")
      .Replace("\"", "&quot;")
      .Replace("'", "&#39;");

  return SafeHTML::unsafelyCreate(script_state, escapedHTML);
}

ScriptPromise SafeHTML::unsafelyCreate(ScriptState* script_state,
                                       const String& html) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  resolver->Resolve(SafeHTML::Create(html));
  return promise;
}

String SafeHTML::toString() const {
  return html_;
}

}  // namespace blink
