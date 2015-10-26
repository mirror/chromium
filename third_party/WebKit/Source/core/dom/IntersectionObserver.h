// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IntersectionObserver_h
#define IntersectionObserver_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "core/dom/Element.h"
#include "core/dom/IntersectionObserverEntry.h"
#include "platform/heap/Handle.h"
#include "wtf/HashSet.h"
#include "wtf/RefCounted.h"
#include "wtf/Vector.h"

namespace blink {

class ExceptionState;
class IntersectionObserverCallback;
class IntersectionObserver;
class IntersectionObserverInit;

using IntersectionObserverEntryVector = WillBeHeapVector<RefPtrWillBeMember<IntersectionObserverEntry>>;
using IntersectionObserverTargetVector = WillBeHeapVector<RefPtrWillBeWeakMember<Element>>;

class IntersectionObserver final : public RefCountedWillBeGarbageCollectedFinalized<IntersectionObserver>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
    friend class Document;
public:
    static PassRefPtrWillBeRawPtr<IntersectionObserver> create(Document*,  const IntersectionObserverInit&, PassOwnPtrWillBeRawPtr<IntersectionObserverCallback>);
    static void resumeSuspendedObservers();

    ~IntersectionObserver();

    void observe(Element*, ExceptionState&);
    void unobserve(Element*, ExceptionState&);
    void disconnect();
    IntersectionObserverEntryVector takeRecords();

    void enqueueIntersectionObserverEntry(PassRefPtrWillBeRawPtr<IntersectionObserverEntry>);

    // Eagerly finalized as destructor accesses heap object members.
    EAGERLY_FINALIZE();
    DECLARE_TRACE();

private:
    explicit IntersectionObserver(Document*,  const IntersectionObserverInit&, PassOwnPtrWillBeRawPtr<IntersectionObserverCallback>);
    void deliver();
    bool shouldBeSuspended() const;

    OwnPtrWillBeMember<IntersectionObserverCallback> m_callback;
    RawPtrWillBeWeakMember<Document> m_document;
    IntersectionObserverEntryVector m_entries;
    IntersectionObserverTargetVector m_targets;
};

} // namespace blink

#endif // IntersectionObserver_h
