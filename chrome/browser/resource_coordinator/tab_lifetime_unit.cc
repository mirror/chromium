// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifetime_unit.h"

#include "base/macros.h"
#include "content/public/browser/web_contents_user_data.h"

namespace resource_coordinator {

namespace {

class TabLifetimeUnitHolder
    : public content::WebContentsUserData<TabLifetimeUnitHolder> {
 public:
  explicit TabLifetimeUnitHolder(content::WebContents*) {}
  TabLifetimeUnit* tab_lifetime_unit() const { return tab_lifetime_unit_; }
  void set_tab_lifetime_unit(TabLifetimeUnit* tab_lifetime_unit) {
    tab_lifetime_unit_ = tab_lifetime_unit;
  }

 private:
  TabLifetimeUnit* tab_lifetime_unit_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TabLifetimeUnitHolder);
};

}  // namespace

// static
TabLifetimeUnit* TabLifetimeUnit::FromWebContents(
    content::WebContents* web_contents) {
  TabLifetimeUnitHolder* holder =
      TabLifetimeUnitHolder::FromWebContents(web_contents);
  return holder ? holder->tab_lifetime_unit() : nullptr;
}

// static
void TabLifetimeUnit::SetForWebContents(content::WebContents* web_contents,
                                        TabLifetimeUnit* tab_lifetime_unit) {
  TabLifetimeUnitHolder::CreateForWebContents(web_contents);
  TabLifetimeUnitHolder::FromWebContents(web_contents)
      ->set_tab_lifetime_unit(tab_lifetime_unit);
}

}  // namespace resource_coordinator

DEFINE_WEB_CONTENTS_USER_DATA_KEY(resource_coordinator::TabLifetimeUnitHolder);
