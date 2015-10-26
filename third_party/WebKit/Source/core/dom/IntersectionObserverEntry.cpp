// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/dom/IntersectionObserverEntry.h"

#include "core/dom/Element.h"

namespace blink {

PassRefPtrWillBeRawPtr<IntersectionObserverEntry> IntersectionObserverEntry::create(double time, const FloatRect& boundingClientRect, const FloatRect& rootBounds, const FloatRect& intersectionRect, Element* target)
{
    return adoptRefWillBeNoop(new IntersectionObserverEntry(time, boundingClientRect, rootBounds, intersectionRect, target));
}

IntersectionObserverEntry::IntersectionObserverEntry(double time, const FloatRect& boundingClientRect, const FloatRect& rootBounds, const FloatRect& intersectionRect, Element* target)
    : m_time(time)
    , m_boundingClientRect(boundingClientRect)
    , m_rootBounds(rootBounds)
    , m_intersectionRect(intersectionRect)
    , m_target(target)

{
}

IntersectionObserverEntry::~IntersectionObserverEntry()
{
}

DEFINE_TRACE(IntersectionObserverEntry)
{
    visitor->trace(m_target);
}


} // namespace blink
