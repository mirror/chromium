// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_DOCUMENT_H_
#define WEBAGENTS_DOCUMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/webagents/node.h"
#include "third_party/WebKit/webagents/webagents_export.h"
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
class WEBAGENTS_EXPORT Document : public Node {
 public:
  // [ImplementedAs=urlForBinding] readonly attribute DOMString URL;
  const blink::WebURL URL() const;
  // readonly attribute Element? documentElement;
  base::Optional<Element> documentElement() const;
  // [CEReactions] attribute HTMLElement? body;
  base::Optional<HTMLElement> body() const;
  // readonly attribute HTMLHeadElement? head;
  base::Optional<HTMLHeadElement> head() const;
  // readonly attribute Element? activeElement;
  base::Optional<Element> activeElement() const;

  // Non-standard
  void UpdateStyleAndLayoutTree() const;

 private:
  friend class Frame;
  explicit Document(blink::Document&);
  blink::Document& GetDocument() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_DOCUMENT_H_
