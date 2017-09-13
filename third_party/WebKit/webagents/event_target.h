// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_EVENT_TARGET_H_
#define WEBAGENTS_EVENT_TARGET_H_

#include "third_party/WebKit/webagents/webagents_export.h"

#include <functional>
#include <memory>

#include "base/callback.h"
#include "base/optional.h"
#include "third_party/WebKit/public/platform/WebPrivatePtr.h"

namespace blink {
class EventTarget;
}

namespace webagents {

class Element;
class Event;
class HTMLElement;
class HTMLFormElement;
class HTMLHeadElement;
class HTMLInputElement;
class HTMLLabelElement;
class HTMLOptionElement;
class HTMLMetaElement;
class HTMLSelectElement;
class HTMLTextAreaElement;
class Text;

// https://dom.spec.whatwg.org/#interface-eventtarget
class WEBAGENTS_EXPORT EventTarget {
 public:
  virtual ~EventTarget();

  EventTarget(const EventTarget& target);
  void Assign(const EventTarget& target);
  bool Equals(const EventTarget& target) const;
  bool LessThan(const EventTarget& target) const;
  EventTarget& operator=(const EventTarget& other) {
    Assign(other);
    return *this;
  }
  friend bool operator==(const EventTarget& lhs, const EventTarget& rhs) {
    return lhs.Equals(rhs);
  }
  friend bool operator!=(const EventTarget& lhs, const EventTarget& rhs) {
    return !lhs.Equals(rhs);
  }
  friend bool operator<(const EventTarget& lhs, const EventTarget& rhs) {
    return lhs.LessThan(rhs);
  }

  // void addEventListener(DOMString type, EventListener? callback, optional
  // (AddEventListenerOptions or boolean) options);
  using EventListener = base::Callback<void(Event)>;
  void addEventListener(std::string, const EventListener) const;

  // Type checking and conversions.
  bool IsCommentNode() const;
  bool IsElementNode() const;
  bool IsTextNode() const;

  bool IsHTMLElement() const;
  bool IsHTMLFormElement() const;
  bool IsHTMLInputElement() const;
  bool IsHTMLLabelElement() const;
  bool IsHTMLMetaElement() const;
  bool IsHTMLNoScriptElement() const;
  bool IsHTMLOptionElement() const;
  bool IsHTMLScriptElement() const;
  bool IsHTMLSelectElement() const;
  bool IsHTMLTextAreaElement() const;

  Element ToElement() const;
  Text ToText() const;

  HTMLElement ToHTMLElement() const;
  HTMLFormElement ToHTMLFormElement() const;
  HTMLHeadElement ToHTMLHeadElement() const;
  HTMLInputElement ToHTMLInputElement() const;
  HTMLLabelElement ToHTMLLabelElement() const;
  HTMLOptionElement ToHTMLOptionElement() const;
  HTMLMetaElement ToHTMLMetaElement() const;
  HTMLSelectElement ToHTMLSelectElement() const;
  HTMLTextAreaElement ToHTMLTextAreaElement() const;

#if INSIDE_BLINK
 protected:
  template <class T>
  T& Unwrap() {
    return static_cast<T&>(*private_);
  }

  template <class T>
  const T& ConstUnwrap() const {
    return static_cast<const T&>(*private_);
  }

  friend class Event;
  explicit EventTarget(blink::EventTarget&);

  template <class Webagent, class Blink>
  base::Optional<Webagent> Create(Blink* ptr) const {
    return ptr ? Webagent(*ptr) : base::Optional<Webagent>();
  }
#endif

 private:
  blink::WebPrivatePtr<blink::EventTarget> private_;
};

}  // namespace webagents

#endif  // WEBAGENTS_EVENT_TARGET_H_
