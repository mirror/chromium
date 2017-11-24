// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/discard_metrics_util.h"

#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "components/metrics/system_memory_stats_recorder.h"

namespace resource_coordinator {

void RecordWillDiscardUrgently(base::TimeTicks discard_time,
                               base::TimeTicks last_urgent_discard_time,
                               base::TimeTicks start_time,
                               int num_alive_tabs) {
  UMA_HISTOGRAM_COUNTS_100("Discarding.Urgent.NumAliveTabs", num_alive_tabs);

  if (last_urgent_discard_time.is_null()) {
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Discarding.Urgent.TimeSinceStartup", discard_time - start_time,
        base::TimeDelta::FromSeconds(1), base::TimeDelta::FromDays(1), 50);
  } else {
    UMA_HISTOGRAM_CUSTOM_TIMES("Discarding.Urgent.TimeSinceLastUrgent",
                               discard_time - last_urgent_discard_time,
                               base::TimeDelta::FromMilliseconds(100),
                               base::TimeDelta::FromDays(1), 50);
  }

// TODO(georgesak): Remove this #if when RecordMemoryStats is implemented for
// all platforms.
#if defined(OS_WIN) || defined(OS_CHROMEOS)
  // Record system memory usage at the time of the discard.
  metrics::RecordMemoryStats(metrics::RECORD_MEMORY_STATS_TAB_DISCARDED);
#endif
}

void RecordTabDiscarded() {
  static int discard_count = 0;
  UMA_HISTOGRAM_CUSTOM_COUNTS("TabManager.Discarding.DiscardCount",
                              ++discard_count, 1, 1000, 50);
}

void RecordTabReloaded(base::TimeTicks last_active_time,
                       base::TimeTicks discard_time,
                       base::TimeTicks reload_time) {
  static int reload_count = 0;
  UMA_HISTOGRAM_CUSTOM_COUNTS("TabManager.Discarding.ReloadCount",
                              ++reload_count, 1, 1000, 50);
  auto discard_to_reload_time = reload_time - discard_time;
  UMA_HISTOGRAM_CUSTOM_TIMES(
      "TabManager.Discarding.DiscardToReloadTime", discard_to_reload_time,
      base::TimeDelta::FromSeconds(1), base::TimeDelta::FromDays(1), 100);
  if (!last_active_time.is_null()) {
    auto inactive_to_reload_time = reload_time - last_active_time;
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "TabManager.Discarding.InactiveToReloadTime", inactive_to_reload_time,
        base::TimeDelta::FromSeconds(1), base::TimeDelta::FromDays(1), 100);
  }
}

}  // namespace resource_coordinator
