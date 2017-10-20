// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_strip_grouper_item.h"

TabStripGrouperItem::TabStripGrouperItem() = default;

TabStripGrouperItem::TabStripGrouperItem(content::WebContents* contents,
                                         int index)
    : type_(Type::TAB), model_index_(index), web_contents_(contents) {}

TabStripGrouperItem::TabStripGrouperItem(const TabStripGrouperItem&) = default;
TabStripGrouperItem::TabStripGrouperItem(TabStripGrouperItem&&) noexcept =
    default;
TabStripGrouperItem::~TabStripGrouperItem() = default;

TabStripGrouperItem& TabStripGrouperItem::operator=(
    const TabStripGrouperItem&) = default;
TabStripGrouperItem& TabStripGrouperItem::operator=(TabStripGrouperItem&&) =
    default;

bool TabStripGrouperItem::operator==(const TabStripGrouperItem& other) const {
  if (type_ != other.type_ || model_index_ != other.model_index_)
    return false;
  if (type_ == Type::TAB)
    return web_contents_ == other.web_contents_;
  return items_ == other.items_;
}
