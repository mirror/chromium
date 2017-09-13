// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_EVENT_H_
#define WEBAGENTS_EVENT_H_

#include "third_party/WebKit/public/platform/WebPrivatePtr.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class Event;
class WebString;
}  // namespace blink

namespace webagents {

class EventTarget;

// https://dom.spec.whatwg.org/#interface-event
class WEBAGENTS_EXPORT Event {
 public:
  virtual ~Event();
  Event(const Event&);

  void Assign(const Event& event);

  // readonly attribute DOMString type;
  blink::WebString type() const;
  // readonly attribute EventTarget? target;
  EventTarget target() const;

#if INSIDE_BLINK
 protected:
  template <class T>
  const T& ConstUnwrap() const {
    return static_cast<const T&>(*private_);
  }

  friend class WebagentsEventListener;
  explicit Event(blink::Event&);
#endif

 private:
  blink::WebPrivatePtr<blink::Event> private_;
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_H_
