// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/CallbackInterfaceCollection.h"

#include "platform/bindings/CallbackInterfaceBase.h"

namespace blink {

DEFINE_TRACE(CallbackInterfaceCollection) {
  visitor->Trace(traceables_);
}

DEFINE_TRACE_WRAPPERS(CallbackInterfaceCollection) {
  for (const auto& traceable : traceables_) {
    visitor->TraceWrappers(traceable);
  }
}

void CallbackInterfaceCollection::RegisterCallbackInterface(
    const CallbackInterfaceBase& traceable) {
  auto result = traceables_.insert(&traceable);
  DCHECK(result.is_new_entry);
}

void CallbackInterfaceCollection::UnregisterCallbackInterface(
    const CallbackInterfaceBase& traceable) {
  DCHECK(traceables_.Contains(&traceable));
  traceables_.erase(&traceable);
}

}  // namespace blink
