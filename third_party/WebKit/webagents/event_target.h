// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_EVENT_TARGET_H_
#define WEBAGENTS_EVENT_TARGET_H_

#include "third_party/WebKit/webagents/webagents_export.h"

#include <functional>
#include <memory>
#include "base/optional.h"

namespace blink {
class EventTarget;
}

namespace webagents {

class Element;
class Event;
class HTMLFormElement;
class HTMLHeadElement;
class HTMLInputElement;
class HTMLMetaElement;
class Text;

// https://dom.spec.whatwg.org/#interface-eventtarget
class WEBAGENTS_EXPORT EventTarget {
 public:
  friend bool operator==(const EventTarget& lhs, const EventTarget& rhs) {
    return &lhs.event_target_ == &rhs.event_target_;
  }
  friend bool operator!=(const EventTarget& lhs, const EventTarget& rhs) {
    return &lhs.event_target_ != &rhs.event_target_;
  }

  // void addEventListener(DOMString type, EventListener? callback, optional
  // (AddEventListenerOptions or boolean) options);
  using EventListener = std::function<void(Event)>;
  void addEventListener(std::string, std::unique_ptr<EventListener>) const;

  // Type checking and conversions.
  bool IsElementNode() const;
  bool IsHTMLInputElement() const;
  bool IsHTMLMetaElement() const;
  bool IsHTMLTextAreaElement() const;
  bool IsTextNode() const;

  Element ToElement() const;
  HTMLFormElement ToHTMLFormElement() const;
  HTMLHeadElement ToHTMLHeadElement() const;
  HTMLInputElement ToHTMLInputElement() const;
  HTMLMetaElement ToHTMLMetaElement() const;
  Text ToText() const;

 protected:
  friend class Event;
  explicit EventTarget(blink::EventTarget&);
  blink::EventTarget& GetEventTarget() const { return *event_target_; }

  template <class Webagent, class Blink>
  base::Optional<Webagent> Create(Blink* ptr) const {
    return ptr ? Webagent(*ptr) : base::Optional<Webagent>();
  }

 private:
  blink::EventTarget* event_target_;
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_TARGET_H_
