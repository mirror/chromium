// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_DOCUMENT_H_
#define WEBAGENTS_DOCUMENT_H_

#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/webagents/node.h"
#include "url/gurl.h"

namespace blink {
class Document;
}

namespace webagents {

class Element;
class HTMLElement;
class HTMLHeadElement;

// https://dom.spec.whatwg.org/#interface-document
// https://html.spec.whatwg.org/#the-document-object
class Document : public Node {
 public:
  virtual ~Document() = default;
  static Document* Create(blink::Document*);

  // [ImplementedAs=urlForBinding] readonly attribute DOMString URL;
  const blink::WebURL URL();
  // readonly attribute Element? documentElement;
  Element* documentElement() const;
  // [CEReactions] attribute HTMLElement? body;
  HTMLElement* body() const;
  // readonly attribute HTMLHeadElement? head;
  const HTMLHeadElement* head() const;

 protected:
  explicit Document(blink::Document*);
  blink::Document* GetDocument() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_DOCUMENT_H_
