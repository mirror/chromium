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
  // TODO(ikilpatrick): This function needs to look at the web developer
  // specified "inputProperties" to potentially invalidate the layout.
  // We will also need to investigate reducing the properties which
  // LayoutBlockFlow::StyleDidChange invalidates upon. (For example margins).

  LayoutBlockFlow::StyleDidChange(diff, old_style);

  // Register if we'll need to reattach the layout tree when a matching
  // "layout()" is registered.
  if (StyleRef().DisplayLayoutCustomState() ==
      EDisplayLayoutCustomState::kUnloaded) {
    LayoutWorklet* worklet = LayoutWorklet::From(*GetDocument().domWindow());
    const AtomicString& name = StyleRef().DisplayLayoutCustomName();
    worklet->AddPendingLayout(name, GetNode());
  }
}

}  // namespace blink
