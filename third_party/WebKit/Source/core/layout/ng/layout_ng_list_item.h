// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef layout_ng_list_item_h
#define layout_ng_list_item_h

#include "core/CoreExport.h"
#include "core/html/ListItemOrdinal.h"
#include "core/layout/ng/layout_ng_block_flow.h"

namespace blink {

// This overrides the default layout block algorithm to use Layout NG.
class CORE_EXPORT LayoutNGListItem final : public LayoutNGBlockFlow {
 public:
  explicit LayoutNGListItem(Element*);

  ListItemOrdinal& Ordinal() { return ordinal_; }

  const char* GetName() const override { return "LayoutNGListItem"; }

 private:
  bool IsOfType(LayoutObjectType) const override;

  void InsertedIntoTree() override;
  void WillBeRemovedFromTree() override;

  ListItemOrdinal ordinal_;
};

DEFINE_LAYOUT_OBJECT_TYPE_CASTS(LayoutNGListItem, IsLayoutNGListItem());

}  // namespace blink

#endif  // layout_ng_list_item_h
