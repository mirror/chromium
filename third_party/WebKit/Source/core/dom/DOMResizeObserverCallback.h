// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DOMResizeObserverCallback_h
#define DOMResizeObserverCallback_h

#include "platform/heap/Handle.h"

namespace blink {

class ResizeObserver;
class ResizeObserverEntry;

class DOMResizeObserverCallback
    : public GarbageCollectedFinalized<DOMResizeObserverCallback> {
 public:
  virtual ~DOMResizeObserverCallback() {}
  virtual void call(const HeapVector<Member<ResizeObserverEntry>>& entries,
                    ResizeObserver*) = 0;
  DEFINE_INLINE_VIRTUAL_TRACE() {}
};

}  // namespace blink

#endif  // DOMResizeObserverCallback_h
