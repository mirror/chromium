// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of https://dom.spec.whatwg.org/#interface-AbortSignal

#ifndef THIRD_PARTY_WEBKIT_SOURCE_CORE_FETCH_ABORTSIGNAL_H_
#define THIRD_PARTY_WEBKIT_SOURCE_CORE_FETCH_ABORTSIGNAL_H_

#include "base/callback_forward.h"
#include "core/dom/events/EventTarget.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/heap/Member.h"
#include "platform/wtf/Vector.h"

namespace blink {

class ExecutionContext;

class AbortSignal final : public EventTargetWithInlineData {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit AbortSignal(ExecutionContext*);
  virtual ~AbortSignal();

  // AbortSignal.idl
  bool aborted() const { return aborted_flag_; }
  DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);

  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;

  // Internal API

  // OnceClosure should use a weak pointer unless the object needs to receive a
  // cancellation signal even after it otherwise would have been destroyed.
  void AddAlgorithm(base::OnceClosure algorithm);

  // Run all algorithms that were added by AddAlgorithm(), in order of addition,
  // then fire an "abort" event. Does nothing if called more than once.
  void SignalAbort();

  virtual void Trace(Visitor*);

 private:
  bool aborted_flag_ = false;
  Vector<base::OnceClosure> abort_algorithms_;
  Member<ExecutionContext> execution_context_;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_CORE_FETCH_ABORTSIGNAL_H_
