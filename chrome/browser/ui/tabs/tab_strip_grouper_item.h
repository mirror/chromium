// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_ITEM_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_ITEM_H_

namespace content {
class WebContents;
}

// One item exposed by the TabStripGrouper. And item can either be a tab or
// a group of tabs.
class TabStripGrouperItem {
 public:
  enum class Type {
    GROUP,
    TAB,
  };

  Type type() const { return type_; }

  

 private:
  Type type_;

  // Valid when type() == GROUP.
  std::vector<tabStripGrouperItem> items_;

  // Valid when type() == TAB.
  content::WebContents* web_contents = nullptr;
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_GROUPER_ITEM_H_
