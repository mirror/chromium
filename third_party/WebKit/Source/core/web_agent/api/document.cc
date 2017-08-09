// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/web_agent/api/document.h"

#include "core/dom/Document.h"
#include "core/web_agent/api/node.h"

namespace web {

Document::Document(blink::Document* document) : Node(document) {}

Document* Document::Create(blink::Document* document) {
  return document ? new Document(document) : nullptr;
}

blink::Document* Document::GetDocument() const {
  return ToDocument(GetNode());
}

const String& Document::URL() const {
  return GetDocument()->Url().GetString();
}

const Element* Document::documentElement() const {
  return Element::Create(GetDocument()->documentElement());
}

}  // namespace web
