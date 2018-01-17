// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptWrappableVisitorVerifier_h
#define ScriptWrappableVisitorVerifier_h

#include "platform/bindings/ScriptWrappableVisitor.h"

namespace blink {

class ScriptWrappableVisitorVerifier final : public ScriptWrappableVisitor {
 public:
  explicit ScriptWrappableVisitorVerifier(v8::Isolate* isolate)
      : ScriptWrappableVisitor(isolate) {}

  void MarkWrappersInAllWorlds(const ScriptWrappable*) const final {}

 protected:
  void Visit(const TraceWrapperV8Reference<v8::Value>&) const final {}
  void Visit(const WrapperDescriptor& wrapper_descriptor,
             const void* traceable) const override {
    HeapObjectHeader* header =
        wrapper_descriptor.heap_object_header_callback(traceable);
    if (ShouldVerify(header)) {
      Verify(wrapper_descriptor, traceable);
    }
  }

 private:
  bool ShouldVerify(HeapObjectHeader* header) const {
    if (!visited_headers_.Contains(header)) {
      visited_headers_.insert(header);
      return true;
    }
    return false;
  }
  void Verify(const WrapperDescriptor& wrapper_descriptor,
              const void* object) const {
    HeapObjectHeader* header =
        wrapper_descriptor.heap_object_header_callback(object);
    if (!header->IsWrapperHeaderMarked()) {
      // If this branch is hit, it means that a white (not discovered by
      // traceWrappers) object was assigned as a member to a black object
      // (already processed by traceWrappers). Black object will not be
      // processed anymore so White object will remain undetected and
      // therefore its wrapper and all wrappers reachable from it would be
      // collected.

      // This means there is a write barrier missing somewhere. Check the
      // backtrace to see which types are causing this and review all the
      // places where white object is set to a black object.
      wrapper_descriptor.missed_write_barrier_callback();
      NOTREACHED();
    }
    wrapper_descriptor.trace_wrappers_callback(this, object);
  }

  mutable WTF::HashSet<HeapObjectHeader*> visited_headers_;
};
}
#endif
