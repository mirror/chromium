// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IntersectionObserverCallback_h
#define IntersectionObserverCallback_h

#include "platform/heap/Handle.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

namespace blink {

class ExecutionContext;
class IntersectionObserver;
class IntersectionObserverEntry;

class IntersectionObserverCallback : public NoBaseWillBeGarbageCollectedFinalized<IntersectionObserverCallback> {
public:
    virtual ~IntersectionObserverCallback() { }

    virtual void handleEvent(WillBeHeapVector<RefPtrWillBeMember<IntersectionObserverEntry>>&, IntersectionObserver*) = 0;
    virtual ExecutionContext* executionContext() const = 0;

    DEFINE_INLINE_VIRTUAL_TRACE() { }
};

}

#endif // IntersectionObserverCallback_h
