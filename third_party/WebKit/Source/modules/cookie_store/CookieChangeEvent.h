// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CookieChangeEvent_h
#define CookieChangeEvent_h

#include "modules/EventModules.h"
#include "modules/cookie_store/CookieListItem.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CookieListItem;
class CookieChangeEventInit;

class CookieChangeEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static CookieChangeEvent* Create() { return new CookieChangeEvent(); }

  // Used by Blink.
  //
  // The caller is expected to create a HeapVector and std::move() it into this
  // method.
  static CookieChangeEvent* Create(const AtomicString& type,
                                   HeapVector<CookieListItem> detail) {
    return new CookieChangeEvent(type, std::move(detail));
  }

  // Used by JavaScript, via the V8 bindings.
  static CookieChangeEvent* Create(const AtomicString& type,
                                   const CookieChangeEventInit& initializer) {
    return new CookieChangeEvent(type, initializer);
  }

  ~CookieChangeEvent() override;

  const HeapVector<CookieListItem>& detail() const { return detail_; }

  // Event
  const AtomicString& InterfaceName() const override;

  // GarbageCollected
  void Trace(blink::Visitor*) override;

 private:
  CookieChangeEvent();
  CookieChangeEvent(const AtomicString& type,
                    HeapVector<CookieListItem> detail);
  CookieChangeEvent(const AtomicString& type,
                    const CookieChangeEventInit& initializer);

  HeapVector<CookieListItem> detail_;
};

}  // namespace blink

#endif  // CookieChangeEvent_h
