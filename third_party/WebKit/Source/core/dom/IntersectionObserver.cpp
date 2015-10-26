// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/dom/IntersectionObserver.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/IntersectionObserverCallback.h"
#include "core/dom/IntersectionObserverEntry.h"
#include "core/dom/IntersectionObserverInit.h"
#include "platform/Timer.h"
#include "wtf/MainThread.h"
#include <algorithm>

namespace blink {

PassRefPtrWillBeRawPtr<IntersectionObserver> IntersectionObserver::create(Document* document, const IntersectionObserverInit& observerInit, PassOwnPtrWillBeRawPtr<IntersectionObserverCallback> callback)
{
    ASSERT(isMainThread());
    return adoptRefWillBeNoop(new IntersectionObserver(document, observerInit, callback));
}

IntersectionObserver::IntersectionObserver(Document* document, const IntersectionObserverInit& observerInit, PassOwnPtrWillBeRawPtr<IntersectionObserverCallback> callback)
    : m_callback(callback)
    , m_document(document)
{
}

IntersectionObserver::~IntersectionObserver()
{
}

void IntersectionObserver::observe(Element* target, ExceptionState& exceptionState)
{
    if (!m_document) {
        exceptionState.throwTypeError("Window may be destroyed? Document is invalid.");
        return;
    }
    if (!target) {
        exceptionState.throwTypeError("Invalid observation target (null).");
        return;
    }

    target->registerIntersectionObserver(this);
    m_targets.append(target);
}

void IntersectionObserver::unobserve(Element* target, ExceptionState& exceptionState)
{
    if (!m_document) {
        exceptionState.throwTypeError("Window may be destroyed? Document is invalid.");
        return;
    }

    if (!target)
        return;

    size_t index = m_targets.find(target);
    if (index == WTF::kNotFound)
        return;

    target->unregisterIntersectionObserver(this);
    m_targets.remove(index);

    if (m_targets.isEmpty())
        m_entries.clear();
}

void IntersectionObserver::disconnect()
{
    m_entries.clear();
    for (auto target : m_targets) {
        if (target)
            target->unregisterIntersectionObserver(this);
    }
    m_targets.clear();
}

IntersectionObserverEntryVector IntersectionObserver::takeRecords()
{
    IntersectionObserverEntryVector entries;
    entries.swap(m_entries);
    return entries;
}

void IntersectionObserver::enqueueIntersectionObserverEntry(PassRefPtrWillBeRawPtr<IntersectionObserverEntry> entry)
{
    ASSERT(isMainThread());
    m_entries.append(entry);
    if (m_document)
        m_document->activateIntersectionObserver(*this);
}

bool IntersectionObserver::shouldBeSuspended() const
{
    return m_callback->executionContext() && m_callback->executionContext()->activeDOMObjectsAreSuspended();
}

void IntersectionObserver::deliver()
{
    ASSERT(!shouldBeSuspended());

    if (m_entries.isEmpty())
        return;

    IntersectionObserverEntryVector entries;
    entries.swap(m_entries);
    m_callback->handleEvent(entries, this);
}

DEFINE_TRACE(IntersectionObserver)
{
    visitor->trace(m_callback);
    visitor->trace(m_document);
    visitor->trace(m_entries);
    visitor->trace(m_targets);
}

} // namespace blink
