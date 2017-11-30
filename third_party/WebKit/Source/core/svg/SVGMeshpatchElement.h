// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGMeshpatchElement_h
#define SVGMeshpatchElement_h

#include "core/svg/SVGElement.h"

namespace blink {

class SVGMeshpatchElement final : public SVGElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  DECLARE_NODE_FACTORY(SVGMeshpatchElement);

 protected:
  // meshpatch elements don't have associated layout objects
  bool LayoutObjectIsNeeded(const ComputedStyle&) override { return false; }

 private:
  explicit SVGMeshpatchElement(Document&);
};

}  // namespace blink

#endif  // SVGMeshpatchElement_h
