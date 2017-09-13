// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_OPTIONS_COLLECTION_H_
#define WEBAGENTS_HTML_OPTIONS_COLLECTION_H_

#include "third_party/WebKit/webagents/html_collection.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLOptionsCollection;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-htmloptionscollection-interface
class WEBAGENTS_EXPORT HTMLOptionsCollection final : public HTMLCollection {
 public:
#if INSIDE_BLINK
 private:
  friend class HTMLSelectElement;
  explicit HTMLOptionsCollection(blink::HTMLOptionsCollection&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_OPTIONS_COLLECTION_H_
