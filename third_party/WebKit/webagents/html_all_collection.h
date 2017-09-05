// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_ALL_COLLECTION_H_
#define WEBAGENTS_HTML_ALL_COLLECTION_H_

#include "third_party/WebKit/webagents/html_collection.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLAllCollection;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-htmlallcollection-interface
class WEBAGENTS_EXPORT HTMLAllCollection : public HTMLCollection {
 private:
  friend class Document;
  explicit HTMLAllCollection(blink::HTMLAllCollection&);
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_ALL_COLLECTION_H_
