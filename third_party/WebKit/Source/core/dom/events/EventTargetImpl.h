// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EventTargetImpl_h
#define EventTargetImpl_h

#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/events/EventTarget.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/ScriptWrappable.h"

namespace blink {

class CORE_EXPORT EventTargetImpl : public EventTargetWithInlineData,
                                    public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(EventTargetImpl);

 public:
  EventTargetImpl(ScriptState*);
  ~EventTargetImpl();

  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;
  void Trace(blink::Visitor*) override;
};

}  // namespace blink

#endif  // EventTargetImpl_h
