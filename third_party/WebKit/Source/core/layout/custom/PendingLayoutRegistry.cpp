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
      if (node) {
        node->SetNeedsStyleRecalc(
            kLocalStyleChange,
            StyleChangeReasonForTracing::Create("CSS Layout API TODO"));
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
