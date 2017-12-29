// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/DocumentAllNameCollection.h"
#include "core/html/HTMLElement.h"

namespace blink {

using namespace HTMLNames;

DocumentAllNameCollection::DocumentAllNameCollection(ContainerNode& document,
                                                     const AtomicString& name)
    : HTMLNameCollection(document, kDocumentAllNamedItems, name) {}

bool DocumentAllNameCollection::ElementMatches(const Element& element) const {
  // https://html.spec.whatwg.org/multipage/common-dom-interfaces.html#all-named-elements
  // Match below type of elements by name but any type of element by id.
  if (element.HasTagName(aTag) || element.HasTagName(appletTag) ||
      element.HasTagName(buttonTag) || element.HasTagName(embedTag) ||
      element.HasTagName(formTag) || element.HasTagName(frameTag) ||
      element.HasTagName(framesetTag) || element.HasTagName(iframeTag) ||
      element.HasTagName(imgTag) || element.HasTagName(inputTag) ||
      element.HasTagName(mapTag) || element.HasTagName(metaTag) ||
      element.HasTagName(objectTag) || element.HasTagName(selectTag) ||
      element.HasTagName(textareaTag)) {
    if (element.GetNameAttribute() == name_)
      return true;
  }

  return element.GetIdAttribute() == name_;
}

}  // namespace blink
