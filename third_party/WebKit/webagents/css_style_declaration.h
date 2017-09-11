// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_CSS_STYLE_DECLARATION_H_
#define WEBAGENTS_CSS_STYLE_DECLARATION_H_

#include "third_party/WebKit/public/platform/WebPrivatePtr.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class CSSStyleDeclaration;
class WebString;
}  // namespace blink

namespace webagents {

// https://drafts.csswg.org/cssom/#the-cssstyledeclaration-interface
class WEBAGENTS_EXPORT CSSStyleDeclaration {
 public:
  virtual ~CSSStyleDeclaration();
  CSSStyleDeclaration(const CSSStyleDeclaration&);

  void Assign(const CSSStyleDeclaration& event);

  // CSSOMString getPropertyValue(CSSOMString property);
  blink::WebString getPropertyValue(blink::WebString property) const;

#if INSIDE_BLINK
 private:
  friend class Element;
  explicit CSSStyleDeclaration(blink::CSSStyleDeclaration&);
#endif

 private:
  blink::WebPrivatePtr<blink::CSSStyleDeclaration> private_;
};

}  // namespace webagents

#endif  // WEBAGENTS_CSS_STYLE_DECLARATION_H_
