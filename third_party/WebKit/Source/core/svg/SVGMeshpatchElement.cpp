// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/svg/SVGMeshpatchElement.h"

namespace blink {

inline SVGMeshpatchElement::SVGMeshpatchElement(Document& document)
    : SVGElement(SVGNames::meshpatchTag, document) {}

DEFINE_NODE_FACTORY(SVGMeshpatchElement)

}  // namespace blink
