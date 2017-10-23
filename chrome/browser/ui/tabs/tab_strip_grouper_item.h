// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_ITEM_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_ITEM_H_

#include <vector>

namespace content {
class WebContents;
}

// One item exposed by the TabStripGrouper. And item can either be a tab or
// a group of tabs.
class TabStripGrouperItem {
 public:
  TabStripGrouperItem();
  TabStripGrouperItem(content::WebContents* contents, int index);

  TabStripGrouperItem(const TabStripGrouperItem&);
  TabStripGrouperItem(TabStripGrouperItem&&) noexcept;
  ~TabStripGrouperItem();

  TabStripGrouperItem& operator=(const TabStripGrouperItem&);
  TabStripGrouperItem& operator=(TabStripGrouperItem&&);

  enum class Type {
    TAB,
    GROUP,
  };

  Type type() const { return type_; }

  int model_index() const { return model_index_; }

  content::WebContents* web_contents() const { return web_contents_; }
  const std::vector<TabStripGrouperItem>& items() const { return items_; }

  bool operator==(const TabStripGrouperItem& other) const;
  inline bool operator!=(const TabStripGrouperItem& other) const {
    return !operator==(other);
  }

  // Returns true if the
  bool TypeAndIndicesEqual(const TabStripGrouperItem& other) const;

 private:
  friend class TabStripGrouper;

  Type type_;

  // Index into the model of this WebContents (for tabs), or the first tab
  // represented (if this is a group).
  int model_index_ = -1;

  // Valid when type() == TAB.
  content::WebContents* web_contents_ = nullptr;

  // Valid when type() == GROUP.
  std::vector<TabStripGrouperItem> items_;
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_ITEM_H_
