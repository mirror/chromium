// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_DOCUMENT_H_
#define WEBAGENTS_DOCUMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/node.h"
#include "third_party/WebKit/webagents/webagents_export.h"
#include "url/gurl.h"

namespace blink {
class Document;
class WebString;
class WebURL;
}  // namespace blink

namespace webagents {

class Element;
class HTMLAllCollection;
class HTMLElement;
class HTMLHeadElement;

// https://dom.spec.whatwg.org/#interface-document
// https://html.spec.whatwg.org/#the-document-object
class WEBAGENTS_EXPORT Document final : public Node {
 public:
  // https://html.spec.whatwg.org/#the-htmlallcollection-interface
  HTMLAllCollection all() const;
  // [ImplementedAs=urlForBinding] readonly attribute DOMString URL;
  blink::WebURL URL() const;
  // readonly attribute Element? documentElement;
  base::Optional<Element> documentElement() const;
  // [CEReactions] attribute DOMString title;
  blink::WebString title() const;
  // [CEReactions] attribute HTMLElement? body;
  base::Optional<HTMLElement> body() const;
  // readonly attribute HTMLHeadElement? head;
  base::Optional<HTMLHeadElement> head() const;
  // readonly attribute Element? activeElement;
  base::Optional<Element> activeElement() const;

  // https://w3c.github.io/webappsec-secure-contexts/#idl-index
  // partial interface WindowOrWorkerGlobalScope {
  //   readonly attribute boolean isSecureContext;
  //   };
  bool isSecureContext() const;
  // https://html.spec.whatwg.org/#form-submission
  // https://html.spec.whatwg.org/#urls
  blink::WebURL CompleteURL(blink::WebString) const;

  // Not standard.
  bool NotStandardIsAttachedToFrame() const;
  void NotStandardUpdateStyleAndLayoutTree();

#if INSIDE_BLINK
 private:
  friend class Node;
  friend class Frame;
  explicit Document(blink::Document&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_DOCUMENT_H_
