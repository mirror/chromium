// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PresentationTurtle_h
#define PresentationTurtle_h

#include "core/frame/DOMWindow.h"
#include "modules/presentation/PresentationConnection.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Member.h"

namespace blink {

class PresentationTurtle final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PresentationTurtle* Create(PresentationConnection*);
  static PresentationTurtle* Create(PresentationConnection*, DOMWindow*);

  PresentationConnection* connection() const;
  DOMWindow* window() const;

  virtual void Trace(blink::Visitor*);

 private:
  explicit PresentationTurtle(PresentationConnection*, DOMWindow*);

  Member<PresentationConnection> connection_;
  Member<DOMWindow> window_;
};

}  // namespace blink

#endif  // PresentationTurtle_h
