// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAccessibleNode_h
#define ComputedAccessibleNode_h

#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/AXObjectCache.h"
#include "core/dom/DOMException.h"
#include "core/dom/events/EventTarget.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/text/AtomicString.h"
#include "third_party/WebKit/Source/core/dom/Element.h"
#include "third_party/WebKit/public/platform/ComputedAXTree.h"

namespace blink {

class ScriptPromiseResolver;
class ScriptState;

class WebFrame;

class ComputedAccessibleNode : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ComputedAccessibleNode* Create(Element*);
  virtual ~ComputedAccessibleNode();

  void Trace(Visitor*);

  // Start compution of the accessibility properties of the stored element and
  // return a promise.
  ScriptPromise ComputeAccessibleProperties(ScriptState*, WebFrame*);

  // Accessors for accessible properties. Can be null.
  const AtomicString role() const;
  const String name() const;

  int32_t colCount() const;
  int32_t colIndex() const;
  int32_t colSpan() const;
  int32_t level() const;
  int32_t posInSet() const;
  int32_t rowCount() const;
  int32_t rowIndex() const;
  int32_t rowSpan() const;
  int32_t setSize() const;

 private:
  explicit ComputedAccessibleNode(Element*);

  // content::ComputedAXTree callback.
  void OnSnapshotResponse(ScriptPromiseResolver*);

  Member<Element> element_;
  Member<AXObjectCache> cache_;
  blink::ComputedAXTree* tree_;
};

}  // namespace blink

#endif  // ComputedAccessibleNode_h
