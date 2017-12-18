// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLayoutOpportunity_h
#define NGLayoutOpportunity_h

#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_bfc_rect.h"

namespace blink {

struct CORE_EXPORT NGLayoutOpportunity {
  // Rectangle in BFC coordinates the represents this opportunity.
  NGBfcRect rect;

  // TODO(ikilpatrick): This will also need to hold all the adjacent exclusions
  // on each edge for implementing css-shapes correctly. It'll need to have
  // methods which use these adjacent exclusions to determine the available
  // inline-size for a line-box.
};

}  // namespace blink

#endif  // NGExclusion_h
