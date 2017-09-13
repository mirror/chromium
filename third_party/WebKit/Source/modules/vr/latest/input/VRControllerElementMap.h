// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VRControllerElementMap_h
#define VRControllerElementMap_h

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/Maplike.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "modules/vr/latest/input/VRControllerElement.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class VRControllerElementMap final
    : public GarbageCollectedFinalized<VRControllerElementMap>,
      public ScriptWrappable,
      public Maplike<String, VRControllerElement*> {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit VRControllerElementMap(
      const HeapHashMap<String, Member<VRControllerElement>>& element_map);

  // IDL attributes / methods
  size_t size() const { return element_map_.size(); }

  VRControllerElement* At(String name) { return element_map_.at(name); }
  bool Contains(String name) { return element_map_.Contains(name); }

  DEFINE_INLINE_VIRTUAL_TRACE() { visitor->Trace(element_map_); }

 private:
  PairIterable<String, VRControllerElement*>::IterationSource* StartIteration(
      ScriptState*,
      ExceptionState&) override;
  bool GetMapEntry(ScriptState*,
                   const String& key,
                   VRControllerElement*&,
                   ExceptionState&) override;

  const HeapHashMap<String, Member<VRControllerElement>> element_map_;
};

}  // namespace blink

#endif  // VRStageBoundsPoint_h
