// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/custom/LayoutCustom.h"

#include "core/dom/Document.h"
#include "core/layout/custom/LayoutWorklet.h"

namespace blink {

LayoutCustom::LayoutCustom(Element* element) : LayoutBlockFlow(element) {
  DCHECK(element);
}

LayoutCustom::~LayoutCustom() = default;

void LayoutCustom::StyleDidChange(StyleDifference diff,
                                  const ComputedStyle* old_style) {
  // TODO(ikilpatrick): Something something scoping.
  LayoutBlockFlow::StyleDidChange(diff, old_style);

  // Register the we will trigger a layout tree reattachment
  if (StyleRef().DisplayLayoutCustomState() ==
      EDisplayLayoutCustomState::kUnloaded) {
    LayoutWorklet* worklet = LayoutWorklet::From(*GetDocument().domWindow());
    const AtomicString& name = StyleRef().DisplayLayoutCustomName();
    worklet->AddPendingLayout(name, GetNode());
  }
}

}  // namespace blink
