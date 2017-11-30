// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGMeshrowElement_h
#define SVGMeshrowElement_h

#include "core/svg/SVGElement.h"

namespace blink {

class SVGMeshrowElement final : public SVGElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  DECLARE_NODE_FACTORY(SVGMeshrowElement);

 protected:
  // meshrow elements don't have associated layout objects
  bool LayoutObjectIsNeeded(const ComputedStyle&) override { return false; }

 private:
  explicit SVGMeshrowElement(Document&);
};

}  // namespace blink

#endif  // SVGMeshrowElement_h
