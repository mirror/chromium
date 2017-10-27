// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ElementComputedStyleMap_h
#define ElementComputedStyleMap_h

#include "core/css/CSSComputedStyleDeclaration.h"
#include "core/css/cssom/ComputedStylePropertyMap.h"

namespace blink {

class ElementComputedStyleMap {
  STATIC_ONLY(ElementComputedStyleMap);

 public:
  static StylePropertyMapReadonly* computedStyleMap(Element& element) {
    // FIXME: Pseudo-element support requires reintroducing Element.pseudo(...).
    // See:
    // https://github.com/w3c/css-houdini-drafts/issues/350#issuecomment-294690156
    return ComputedStylePropertyMap::Create(&element, "");
  }
};

}  // namespace blink

#endif  // ElementComputedStyleMap_h
