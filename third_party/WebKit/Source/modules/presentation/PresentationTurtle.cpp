// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/presentation/PresentationTurtle.h"

namespace blink {

// static
PresentationTurtle* PresentationTurtle::Create(
    PresentationConnection* connection) {
  return new PresentationTurtle(connection, nullptr);
}

// static
PresentationTurtle* PresentationTurtle::Create(
    PresentationConnection* connection,
    DOMWindow* window) {
  return new PresentationTurtle(connection, window);
}

PresentationConnection* PresentationTurtle::connection() const {
  return connection_;
}

DOMWindow* PresentationTurtle::window() const {
  return window_;
}

void PresentationTurtle::Trace(blink::Visitor* visitor) {
  visitor->Trace(connection_);
  visitor->Trace(window_);
  ScriptWrappable::Trace(visitor);
}

PresentationTurtle::PresentationTurtle(PresentationConnection* connection,
                                       DOMWindow* window)
    : connection_(connection), window_(window) {}
}  // namespace blink
