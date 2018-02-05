// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/custom/LayoutCustom.h"

#include "core/dom/Document.h"
#include "core/layout/custom/LayoutWorklet.h"

namespace blink {

LayoutCustom::LayoutCustom(Element* element)
    : LayoutBlockFlow(element), is_loaded_(false) {
  DCHECK(element);
}

LayoutCustom::~LayoutCustom() = default;

void LayoutCustom::StyleDidChange(StyleDifference diff,
                                  const ComputedStyle* old_style) {
  // TODO(ikilpatrick): Something something scoping.
  LayoutBlockFlow::StyleDidChange(diff, old_style);

  LayoutWorklet* worklet = LayoutWorklet::From(*GetDocument().domWindow());
  const AtomicString& name = StyleRef().DisplayLayoutCustomName();

  is_loaded_ = worklet->GetDocumentDefinitionMap()->Contains(name);
  if (!is_loaded_) {
    worklet->AddPendingLayout(name, GetNode());
    //  } else {
    //    DCHECK(!ChildrenInline());
  }
}

}  // namespace blink
