// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/document.h"

#include "third_party/WebKit/Source/core/dom/Document.h"
#include "third_party/WebKit/webagents/node.h"

namespace webagents {

Document::Document(blink::Document* document) : Node(document) {}

Document* Document::Create(blink::Document* document) {
  return document ? new Document(document) : nullptr;
}

blink::Document* Document::GetDocument() const {
  return ToDocument(GetNode());
}

const GURL* Document::URL() const {
  // return new GURL(GetDocument()->Url());
  return nullptr;
}

const Element* Document::documentElement() const {
  return Element::Create(GetDocument()->documentElement());
}

}  // namespace webagents
