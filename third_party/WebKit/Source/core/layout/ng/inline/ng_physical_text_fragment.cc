// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_physical_text_fragment.h"

#include "core/layout/ng/inline/ng_inline_node.h"

namespace blink {

StringView NGPhysicalTextFragment::Text() const {
  return node_.Text(start_offset_, end_offset_);
}

}  // namespace blink
