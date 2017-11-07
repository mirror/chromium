// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/CSSPaintWorklet.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/LocalDOMWindow.h"
#include "modules/csspaint/PaintWorklet.h"
#include "platform/bindings/ScriptState.h"

namespace blink {

// static
Worklet* CSSPaintWorklet::paintWorklet(ScriptState* script_state) {
  // TODO(nhiroki): Replace this with the [SecureContext] attribute when it's
  // supported (https://crbug.com/782121)
  String error_message;
  if (!ExecutionContext::From(script_state)->IsSecureContext(error_message))
    return nullptr;
  return PaintWorklet::From(*ToLocalDOMWindow(script_state->GetContext()));
}

}  // namespace blink
