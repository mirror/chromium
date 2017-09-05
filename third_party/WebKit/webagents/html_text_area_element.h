// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_TEXT_AREA_ELEMENT_H_
#define WEBAGENTS_HTML_TEXT_AREA_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLTextAreaElement;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-textarea-element
class WEBAGENTS_EXPORT HTMLTextAreaElement : public HTMLElement {
 public:
  // [CEReactions] attribute boolean disabled;
  bool disabled() const;
  // [CEReactions] attribute boolean readOnly;
  bool readOnly() const;

  // Not standard.
  std::string NotStandardEditingValue() const;
  int NotStandardSelectionStart() const;
  int NotStandardSelectionEnd() const;
  std::string NotStandardSuggestedValue() const;
 private:
  friend class EventTarget;
  explicit HTMLTextAreaElement(blink::HTMLTextAreaElement&);
  blink::HTMLTextAreaElement& GetHTMLTextAreaElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_TEXT_AREA_ELEMENT_H_
