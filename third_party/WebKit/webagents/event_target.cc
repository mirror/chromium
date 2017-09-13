// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/event_target.h"

#include "core/HTMLElementTypeHelpers.h"
#include "core/dom/Text.h"
#include "core/dom/events/EventTarget.h"
#include "core/html/HTMLFormElement.h"
#include "core/html/HTMLHeadElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/HTMLLabelElement.h"
#include "core/html/HTMLMetaElement.h"
#include "core/html/HTMLSelectElement.h"
#include "core/html/HTMLTextAreaElement.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/webagents/event.h"
#include "third_party/WebKit/webagents/html_form_element.h"
#include "third_party/WebKit/webagents/html_head_element.h"
#include "third_party/WebKit/webagents/html_input_element.h"
#include "third_party/WebKit/webagents/html_label_element.h"
#include "third_party/WebKit/webagents/html_meta_element.h"
#include "third_party/WebKit/webagents/html_option_element.h"
#include "third_party/WebKit/webagents/html_select_element.h"
#include "third_party/WebKit/webagents/html_text_area_element.h"
#include "third_party/WebKit/webagents/text.h"

namespace webagents {

EventTarget::~EventTarget() {
  private_.Reset();
}

EventTarget::EventTarget(blink::EventTarget& target) : private_(&target) {}

EventTarget::EventTarget(const EventTarget& target) {
  Assign(target);
}

void EventTarget::Assign(const EventTarget& target) {
  private_ = target.private_;
}

bool EventTarget::Equals(const EventTarget& target) const {
  return private_.Get() == target.private_.Get();
}

bool EventTarget::LessThan(const EventTarget& target) const {
  return private_.Get() < target.private_.Get();
}

class WebagentsEventListener : public blink::EventListener {
 public:
  explicit WebagentsEventListener(const EventTarget::EventListener callback)
      : EventListener(kCPPEventListenerType), callback_(std::move(callback)) {}
  void handleEvent(blink::ExecutionContext*, blink::Event* event) override {
    callback_.Run(Event(*event));
  }
  bool operator==(const blink::EventListener& other) const override {
    return this == &other;
  }

 private:
  const EventTarget::EventListener callback_;
};

void EventTarget::addEventListener(std::string type,
                                   const EventListener callback) const {
  private_->addEventListener(blink::WebString::FromASCII(type),
                             new WebagentsEventListener(std::move(callback)));
}

// Type checking and conversions.
#define IS_HTML_ELEMENT(type)                          \
  bool EventTarget::IsHTML##type##Element() const {    \
    return isHTML##type##Element(*private_->ToNode()); \
  }

#define TO_HTML_ELEMENT(type)                                               \
  HTML##type##Element EventTarget::ToHTML##type##Element() const {          \
    return HTML##type##Element(toHTML##type##Element(*private_->ToNode())); \
  }

bool EventTarget::IsElementNode() const {
  return private_->ToNode()->IsElementNode();
}
bool EventTarget::IsCommentNode() const {
  return private_->ToNode()->getNodeType() == blink::Node::kCommentNode;
}
bool EventTarget::IsTextNode() const {
  return private_->ToNode()->IsTextNode();
}
bool EventTarget::IsHTMLElement() const {
  return private_->ToNode()->IsHTMLElement();
}

IS_HTML_ELEMENT(Form);
IS_HTML_ELEMENT(Input);
IS_HTML_ELEMENT(Label);
IS_HTML_ELEMENT(Meta);
IS_HTML_ELEMENT(NoScript);
IS_HTML_ELEMENT(Option);
IS_HTML_ELEMENT(Script);
IS_HTML_ELEMENT(Select);
IS_HTML_ELEMENT(TextArea);

Element EventTarget::ToElement() const {
  return Element(blink::ToElement(*private_->ToNode()));
}
HTMLElement EventTarget::ToHTMLElement() const {
  return HTMLElement(blink::ToHTMLElement(*private_->ToNode()));
}
Text EventTarget::ToText() const {
  return Text(blink::ToText(*private_->ToNode()));
}

TO_HTML_ELEMENT(Form);
TO_HTML_ELEMENT(Head);
TO_HTML_ELEMENT(Input);
TO_HTML_ELEMENT(Label);
TO_HTML_ELEMENT(Option);
TO_HTML_ELEMENT(Meta);
TO_HTML_ELEMENT(Select);
TO_HTML_ELEMENT(TextArea);

}  // namespace webagents
