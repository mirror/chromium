// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_renderer_data.h"

#include "base/process/kill.h"
#include "build/build_config.h"

TabRendererData::TabRendererData() = default;
TabRendererData::TabRendererData(const TabRendererData& other) = default;
TabRendererData::TabRendererData(TabRendererData&& other) = default;

TabRendererData& TabRendererData::operator=(const TabRendererData& other) =
    default;
TabRendererData& TabRendererData::operator=(TabRendererData&& other) = default;

TabRendererData::~TabRendererData() = default;

bool operator==(const TabRendererData& a, const TabRendererData& b) {
  return a.favicon.BackedBySameObjectAs(b.favicon) &&
         a.network_state == b.network_state && a.title == b.title &&
         a.url == b.url && a.crashed_status == b.crashed_status &&
         a.incognito == b.incognito && a.show_icon == b.show_icon &&
         a.pinned == b.pinned && a.blocked == b.blocked && a.app == b.app &&
         a.alert_state == b.alert_state;
}

bool TabRendererData::IsCrashed() const {
  return (crashed_status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED ||
#if defined(OS_CHROMEOS)
          crashed_status ==
              base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM ||
#endif
          crashed_status == base::TERMINATION_STATUS_PROCESS_CRASHED ||
          crashed_status == base::TERMINATION_STATUS_ABNORMAL_TERMINATION ||
          crashed_status == base::TERMINATION_STATUS_LAUNCH_FAILED);
}
