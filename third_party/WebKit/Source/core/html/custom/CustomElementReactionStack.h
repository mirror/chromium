// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CustomElementReactionStack_h
#define CustomElementReactionStack_h

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class CustomElementReaction;
class CustomElementReactionQueue;
class Element;

// https://html.spec.whatwg.org/multipage/scripting.html#custom-element-reactions
class CORE_EXPORT CustomElementReactionStack final
    : public GarbageCollected<CustomElementReactionStack>,
      public TraceWrapperBase {
  WTF_MAKE_NONCOPYABLE(CustomElementReactionStack);

 public:
  CustomElementReactionStack();

  DECLARE_TRACE();
  DECLARE_VIRTUAL_TRACE_WRAPPERS();

  void Push();
  void PopInvokingReactions();
  void EnqueueToCurrentQueue(Element*, CustomElementReaction*);
  void EnqueueToBackupQueue(Element*, CustomElementReaction*);
  void ClearQueue(Element*);

  // Gets the custom element reaction stack. This object is lazily
  // created.
  static CustomElementReactionStack& Current();

  // Gets the CustomElementReactionStack if it exists. Although
  // there's technically always a custom element reaction stack, the
  // object is created lazily. For callers which do not need to use
  // the stack they can use MaybeCurrent and treat a nullptr as an
  // empty stack.
  static CustomElementReactionStack* MaybeCurrent();

 private:
  friend class CustomElementReactionStackTestSupport;

  using ElementReactionQueueMap =
      HeapHashMap<TraceWrapperMember<Element>,
                  Member<CustomElementReactionQueue>>;
  ElementReactionQueueMap map_;

  using ElementQueue = HeapVector<Member<Element>, 1>;
  HeapVector<Member<ElementQueue>> stack_;
  Member<ElementQueue> backup_queue_;

  void InvokeBackupQueue();
  void InvokeReactions(ElementQueue&);
  void Enqueue(Member<ElementQueue>&, Element*, CustomElementReaction*);
};

class CORE_EXPORT CustomElementReactionStackTestSupport final {
 private:
  friend class ResetCustomElementReactionStackForTest;

  CustomElementReactionStackTestSupport() = delete;
  static CustomElementReactionStack* SetCurrentForTest(
      CustomElementReactionStack*);
};

}  // namespace blink

#endif  // CustomElementReactionStack_h
