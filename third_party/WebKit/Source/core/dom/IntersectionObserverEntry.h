// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IntersectionObserverEntry_h
#define IntersectionObserverEntry_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "core/dom/ClientRect.h"
#include "platform/geometry/FloatRect.h"
#include "platform/heap/Handle.h"

namespace blink {

class Element;

class IntersectionObserverEntry : public RefCountedWillBeGarbageCollectedFinalized<IntersectionObserverEntry>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PassRefPtrWillBeRawPtr<IntersectionObserverEntry> create(double, const FloatRect&, const FloatRect&, const FloatRect&, Element*);

    ~IntersectionObserverEntry();

    double time() { return m_time; }
    ClientRect* boundingClientRect() { return ClientRect::create(m_boundingClientRect); }
    ClientRect* rootBounds() { return ClientRect::create(m_rootBounds); }
    ClientRect* intersectionRect() { return ClientRect::create(m_intersectionRect); }
    Element* target() { return m_target.get(); }

    DECLARE_VIRTUAL_TRACE();

protected:
    IntersectionObserverEntry(double, const FloatRect&, const FloatRect&, const FloatRect&, Element*);

private:
    const double m_time;
    const FloatRect m_boundingClientRect;
    const FloatRect m_rootBounds;
    const FloatRect m_intersectionRect;
    const RefPtrWillBeMember<Element> m_target;
};

} // namespace blink

#endif // IntersectionObserverEntry_h
