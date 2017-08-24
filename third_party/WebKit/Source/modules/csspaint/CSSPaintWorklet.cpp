// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/CSSPaintWorklet.h"

#include "core/css/DOMWindowCSS.h"
#include "core/dom/Document.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "modules/csspaint/PaintWorklet.h"

namespace blink {

// static
Worklet* CSSPaintWorklet::paintWorklet(ScriptState* script_state) {
  return paintWorklet(*ToDocument(ExecutionContext::From(script_state)));
}

// static
PaintWorklet* CSSPaintWorklet::paintWorklet(const Document& document) {
  return PaintWorklet::From(*document.ExecutingWindow());
}

}  // namespace blink
