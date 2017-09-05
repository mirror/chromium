// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_COLLECTION_H_
#define WEBAGENTS_HTML_COLLECTION_H_

#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLCollection;
}

namespace webagents {

// https://dom.spec.whatwg.org/#interface-htmlcollection
class WEBAGENTS_EXPORT HTMLCollection {
 public:

 protected:
  explicit HTMLCollection(blink::HTMLCollection&);
  blink::HTMLCollection& GetHTMLCollection() const { return *collection_; }

 private:
  // TODO(joelhockey): Does this need to be WebPrivatePtr?
  blink::HTMLCollection* collection_;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_COLLECTION_H_
