// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/custom/PendingLayoutRegistry.h"

#include "core/css/StyleChangeReason.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"

namespace blink {

void PendingLayoutRegistry::NotifyLayoutReady(const AtomicString& name) {
  PendingSet* set = pending_layouts_.at(name);
  if (set) {
    for (const auto& node : *set) {
      // If the node hasn't been gc'd, trigger a style-recalc so that the
      // layout tree gets reattached if necessary.
      //
      // NOTE: From the time when this node was added as having a pending
      // layout, its display value may have changed to something (block) which
      // doesn't need a layout tree reattachment.
      if (node) {
        node->SetNeedsStyleRecalc(
            kLocalStyleChange,
            StyleChangeReasonForTracing::Create(
                StyleChangeReason::kCSSLayoutAPIRegistration));
      }
    }
  }
  pending_layouts_.erase(name);
}

void PendingLayoutRegistry::AddPendingLayout(const AtomicString& name,
                                             Node* node) {
  Member<PendingSet>& set =
      pending_layouts_.insert(name, nullptr).stored_value->value;
  if (!set)
    set = new PendingSet;
  set->insert(node);
}

void PendingLayoutRegistry::Trace(blink::Visitor* visitor) {
  visitor->Trace(pending_layouts_);
}

}  // namespace blink
