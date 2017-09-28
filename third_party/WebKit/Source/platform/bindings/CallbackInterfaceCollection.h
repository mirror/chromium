// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CallbackInterfaceCollection_h
#define CallbackInterfaceCollection_h

#include "platform/bindings/TraceWrapperMember.h"
#include "platform/heap/HeapAllocator.h"

namespace blink {

class CallbackInterfaceBase;

// 
class PLATFORM_EXPORT CallbackInterfaceCollection final
    : public GarbageCollectedFinalized<CallbackInterfaceCollection>,
      public TraceWrapperBase {
 public:
  static CallbackInterfaceCollection* Create() {
    return new CallbackInterfaceCollection();
  }

  ~CallbackInterfaceCollection() = default;

  DECLARE_TRACE();
  DECLARE_TRACE_WRAPPERS();

  // Registers and unregisters a callback interface object in order to keep
  // it alive.
  void RegisterCallbackInterface(const CallbackInterfaceBase&);
  void UnregisterCallbackInterface(const CallbackInterfaceBase&);

 private:
  CallbackInterfaceCollection() = default;

  HeapHashSet<TraceWrapperMember<const CallbackInterfaceBase>> traceables_;
};

}  // namespace blink

#endif  // CallbackInterfaceCollection_h
