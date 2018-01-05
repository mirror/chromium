// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAccessibleNode_h
#define ComputedAccessibleNode_h

#include "bindings/core/v8/ScriptPromiseProperty.h"
#include "core/dom/DOMException.h"
#include "core/dom/events/EventTarget.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/text/AtomicString.h"

class PlatformAXTree;
class ScriptPromise;
class ScriptState;

namespace blink {

class ComputedAccessibleNode : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ComputedAccessibleNode* Create(Element*);
  virtual ~ComputedAccessibleNode();

  // Start compution of the accessibility properties of the stored element and
  // return a promise.
  ScriptPromise ComputePromiseProperty(ScriptState*);

  void Trace(blink::Visitor*);

  const AtomicString& role() const;
  const String name() const;

 private:
  using ComputedPromiseProperty =
      ScriptPromiseProperty<Member<ComputedAccessibleNode>,
                            Member<ComputedAccessibleNode>,
                            Member<DOMException>>;

  explicit ComputedAccessibleNode(Element*);

  // Given a snapshot of the AXTree stored in content/renderer, populate the
  // accessible properties stored within this ComputedAccessibleNode.
  PopulateAccessibleProperties(PlatformAXTree*);

  Member<ComputedPromiseProperty> computed_property_;
  Member<Element> element_;
};

}  // namespace blink

#endif  // ComputedAccessibleNode_h
