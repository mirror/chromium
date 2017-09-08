// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_AUTOFILL_ELEMENT_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_AUTOFILL_ELEMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_form_element.h"

namespace webagents {
class Element;
class HTMLFormElement;
class HTMLInputElement;
class HTMLTextAreaElement;
class HTMLSelectElement;
}  // namespace webagents

namespace autofill {

class AutofillElement {
 public:
  ~AutofillElement();
  AutofillElement(const AutofillElement&);
  AutofillElement(const webagents::HTMLInputElement element);
  AutofillElement(const webagents::HTMLTextAreaElement element);
  AutofillElement(const webagents::HTMLSelectElement element);

  static base::Optional<AutofillElement> FromElement(
      webagents::Element element);

  const webagents::Element* operator->() const { return &element_; }
  const webagents::Element& operator*() const { return element_; }
  bool operator==(const AutofillElement& rhs) const {
    return element_ == rhs.element_;
  }
  bool operator<(const AutofillElement& rhs) const {
    return element_ < rhs.element_;
  }

  bool disabled() const { return disabled_; }
  base::Optional<webagents::HTMLFormElement> form() const { return form_; }
  blink::WebString name() const { return name_; }
  bool readOnly() const { return readOnly_; }
  blink::WebString type() const { return type_; }
  blink::WebString value() const { return value_; }
  blink::WebString suggestedValue() const { return suggested_value_; }
  blink::WebString editingValue() const { return editing_value_; }
  int selectionStart() const { return selection_start_; }
  int selectionEnd() const { return selection_end_; }

 private:
  AutofillElement(webagents::Element element,
                  bool disabled,
                  base::Optional<webagents::HTMLFormElement> form,
                  blink::WebString name,
                  bool readOnly,
                  blink::WebString type,
                  blink::WebString value,
                  blink::WebString suggested_value,
                  blink::WebString editing_value,
                  int suggested_start,
                  int suggested_end);

  const webagents::Element element_;
  const bool disabled_;
  const base::Optional<webagents::HTMLFormElement> form_;
  const blink::WebString name_;
  const bool readOnly_;
  const blink::WebString type_;
  const blink::WebString value_;
  const blink::WebString suggested_value_;
  const blink::WebString editing_value_;
  const int selection_start_;
  const int selection_end_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_AUTOFILL_ELEMENT_H_
