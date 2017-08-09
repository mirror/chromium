// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_AGENT_API_DOCUMENT_H_
#define WEB_AGENT_API_DOCUMENT_H_

#include "core/web_agent/api/element.h"
#include "core/web_agent/api/node.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {
class Document;
}

namespace web {
class Document : public Node {
 public:
  virtual ~Document() = default;
  static Document* Create(blink::Document*);

  // [ImplementedAs=urlForBinding] readonly attribute DOMString URL;
  const String& URL() const;

  // readonly attribute Element? documentElement;
  const Element* documentElement() const;

 protected:
  explicit Document(blink::Document*);
  blink::Document* GetDocument() const;
};
}  // namespace web
#endif  // WEB_AGENT_API_DOCUMENT_H_
