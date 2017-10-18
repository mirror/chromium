// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_STATS_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_STATS_H_

#include <stdint.h>

#include <vector>

#include "base/process/process.h"
#include "build/build_config.h"

namespace content {
class RenderProcessHost;
}  // namespace content

namespace resource_coordinator {

struct TabStats {
  TabStats();
  TabStats(const TabStats& other);
  TabStats(TabStats&& other) noexcept;
  ~TabStats();

  TabStats& operator=(const TabStats& other);

  content::RenderProcessHost* render_process_host = nullptr;
  base::ProcessHandle renderer_handle = 0;
  int child_process_host_id = 0;
  int64_t tab_contents_id = 0;  // Unique ID per WebContents.
};

typedef std::vector<TabStats> TabStatsList;

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_STATS_H_
