// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/svg/SVGMeshrowElement.h"

namespace blink {

inline SVGMeshrowElement::SVGMeshrowElement(Document& document)
    : SVGElement(SVGNames::meshrowTag, document) {}

DEFINE_NODE_FACTORY(SVGMeshrowElement)

}  // namespace blink
