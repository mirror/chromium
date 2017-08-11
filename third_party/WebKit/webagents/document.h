// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_DOCUMENT_H_
#define WEBAGENTS_DOCUMENT_H_

#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/node.h"
#include "url/gurl.h"

namespace blink {
class Document;
}

namespace webagents {
class Document : public Node {
 public:
  virtual ~Document() = default;
  static Document* Create(blink::Document*);

  // [ImplementedAs=urlForBinding] readonly attribute DOMString URL;
  const GURL* URL() const;

  // readonly attribute Element? documentElement;
  const Element* documentElement() const;

 protected:
  explicit Document(blink::Document*);
  blink::Document* GetDocument() const;
};
}  // namespace webagents
#endif  // WEBAGENTS_DOCUMENT_H_
