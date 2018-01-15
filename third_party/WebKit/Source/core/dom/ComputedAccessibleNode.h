// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAccessibleNode_h
#define ComputedAccessibleNode_h

#include "core/dom/DOMException.h"
#include "core/dom/events/EventTarget.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/wtf/text/AtomicString.h"
#include "third_party/WebKit/Source/core/dom/AXObjectCache.h"
#include "third_party/WebKit/public/platform/ComputedAXTree.h"

class ScriptPromise;
class ScriptPromiseResolver;
class ScriptState;

namespace blink {

class WebFrame;

typedef unsigned AXID;

class ComputedAccessibleNode : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ComputedAccessibleNode* Create(AXID);
  virtual ~ComputedAccessibleNode();

  // Start compution of the accessibility properties of the stored element and
  // return a promise.
  ScriptPromise ComputeAccessibleProperties(ScriptState*, WebFrame*);

  // Accessors for accessible properties. Can be null.
  const AtomicString role() const;
  const String name() const;

 private:
  explicit ComputedAccessibleNode(AXID);

  // content::ComputedAXTree callback.
  void OnSnapshotResponse(ScriptPromiseResolver*);

  AXID id_;
  blink::ComputedAXTree* tree_;
};

}  // namespace blink

#endif  // ComputedAccessibleNode_h
