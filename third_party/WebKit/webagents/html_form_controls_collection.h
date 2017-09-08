// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_FORM_CONTROLS_COLLECTION_H_
#define WEBAGENTS_HTML_FORM_CONTROLS_COLLECTION_H_

#include "third_party/WebKit/webagents/html_collection.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLFormControlsCollection;
}

namespace webagents {

// https://html.spec.whatwg.org/#the-htmlformcontrolscollection-interface
class WEBAGENTS_EXPORT HTMLFormControlsCollection : public HTMLCollection {
 private:
  friend class HTMLFormElement;
  explicit HTMLFormControlsCollection(blink::HTMLFormControlsCollection&);
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_FORM_CONTROLS_COLLECTION_H_
