// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/layout_ng_list_item.h"

namespace blink {

LayoutNGListItem::LayoutNGListItem(Element* element)
    : LayoutNGBlockFlow(element) {
  SetInline(false);
}

bool LayoutNGListItem::IsOfType(LayoutObjectType type) const {
  return type == kLayoutObjectNGListItem || LayoutNGBlockFlow::IsOfType(type);
}

void LayoutNGListItem::InsertedIntoTree() {
  LayoutNGBlockFlow::InsertedIntoTree();

  ListItemOrdinal::ItemInsertedOrRemoved(this);
}

void LayoutNGListItem::WillBeRemovedFromTree() {
  LayoutNGBlockFlow::WillBeRemovedFromTree();

  ListItemOrdinal::ItemInsertedOrRemoved(this);
}

}  // namespace blink
