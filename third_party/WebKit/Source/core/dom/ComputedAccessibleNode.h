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
enum AOMIntAttribute;

class ComputedAccessibleNode : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ComputedAccessibleNode* Create(Element*);
  virtual ~ComputedAccessibleNode();

  void Trace(Visitor*);

  // Starts computation of the accessibility properties of the stored element.
  ScriptPromise ComputeAccessibleProperties(ScriptState*, WebFrame*);

  // String attribute accessors. These can return blank strings if the element
  // has no node in the computed accessible tree, or property does not apply.
  const AtomicString role() const;
  const String name() const;

  int32_t colCount(bool& is_null) const;
  int32_t colIndex(bool& is_null) const;
  int32_t colSpan(bool& is_null) const;
  int32_t level(bool& is_null) const;
  int32_t posInSet(bool& is_null) const;
  int32_t rowCount(bool& is_null) const;
  int32_t rowIndex(bool& is_null) const;
  int32_t rowSpan(bool& is_null) const;
  int32_t setSize(bool& is_null) const;

 private:
  explicit ComputedAccessibleNode(Element*);

  // content::ComputedAXTree callback.
  void OnSnapshotResponse(ScriptPromiseResolver*);

  int32_t GetProperty(AOMIntAttribute, int32_t*, bool&) const;

  Member<Element> element_;
  Member<AXObjectCache> cache_;
  blink::ComputedAXTree* tree_;
};

}  // namespace blink

#endif  // ComputedAccessibleNode_h
