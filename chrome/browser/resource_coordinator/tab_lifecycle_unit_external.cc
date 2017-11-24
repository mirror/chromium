// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"

#include "base/macros.h"
#include "content/public/browser/web_contents_user_data.h"

namespace resource_coordinator {

namespace {

class TabLifecycleUnitExternalHolder
    : public content::WebContentsUserData<TabLifecycleUnitExternalHolder> {
 public:
  explicit TabLifecycleUnitExternalHolder(content::WebContents*) {}
  TabLifecycleUnitExternal* tab_lifecycle_unit_external() const {
    return tab_lifecycle_unit_external_;
  }
  void set_tab_lifecycle_unit_external(
      TabLifecycleUnitExternal* tab_lifecycle_unit_external) {
    tab_lifecycle_unit_external_ = tab_lifecycle_unit_external;
  }

 private:
  TabLifecycleUnitExternal* tab_lifecycle_unit_external_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TabLifecycleUnitExternalHolder);
};

}  // namespace

// static
TabLifecycleUnitExternal* TabLifecycleUnitExternal::FromWebContents(
    content::WebContents* web_contents) {
  TabLifecycleUnitExternalHolder* holder =
      TabLifecycleUnitExternalHolder::FromWebContents(web_contents);
  return holder ? holder->tab_lifecycle_unit_external() : nullptr;
}

// static
void TabLifecycleUnitExternal::SetForWebContents(
    content::WebContents* web_contents,
    TabLifecycleUnitExternal* tab_lifecycle_unit_external) {
  TabLifecycleUnitExternalHolder::CreateForWebContents(web_contents);
  TabLifecycleUnitExternalHolder::FromWebContents(web_contents)
      ->set_tab_lifecycle_unit_external(tab_lifecycle_unit_external);
}

}  // namespace resource_coordinator

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    resource_coordinator::TabLifecycleUnitExternalHolder);
