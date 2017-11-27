// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_FEATURES_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_FEATURES_H_

#include "base/feature_list.h"
#include "base/time/time.h"

namespace features {

extern const base::Feature kCustomizedTabLoadTimeout;
extern const base::Feature kProactiveTabDiscarding;
extern const base::Feature kStaggeredBackgroundTabOpening;
extern const base::Feature kStaggeredBackgroundTabOpeningExperiment;

}  // namespace features

namespace resource_coordinator {

// Variations parameter names related to proactive discarding.
// See ProactiveTabDiscardsParams for details.
extern const char kProactiveTabDiscard_MaxLoadedTabsPerGbRamParam[];
extern const char kProactiveTabDiscard_MinLoadedTabCountParam[];
extern const char kProactiveTabDiscard_MaxLoadedTabCountParam[];
extern const char kProactiveTabDiscard_MinOccludedTimeoutParam[];
extern const char kProactiveTabDiscard_MaxOccludedTimeoutParam[];

// Default values of parameters related to proactive discarding.
// See ProactiveTabDiscardsParams for details.
extern const uint32_t kProactiveTabDiscard_MaxLoadedTabsPerGbRamDefault;
extern const uint32_t kProactiveTabDiscard_MinLoadedTabCountDefault;
extern const uint32_t kProactiveTabDiscard_MaxLoadedTabCountDefault;
extern const base::TimeDelta kProactiveTabDiscard_MinOccludedTimeoutDefault;
extern const base::TimeDelta kProactiveTabDiscard_MaxOccludedTimeoutDefault;

// Parameters used by the proactive tab discarding feature.
//
// Proactive discarding has 4 key parameters:
//
// - min/max occluded timeouts
// - min/max loaded tab number
//
// Proactive tab discarding decisions are made at two points in time:
//
// - when a new tab is created
// - when a max occluded timeout fires
//
// The following invariant is maintained:
//
// - no tabs will be discarded if fewer than 'min loaded tabs' are present.
// - no tab will be discarded unless it has been occluded for
//   'min occluded timeout'
// - any tab beyond 'min loaded tab' that has been occluded for
//   'max occluded timeout' is discarded.
// - tabs that have been occluded beyond 'min occluded timeout' are eligible
//   for discarding until at most 'max tab number' loaded tabs remain.
//
// In this way no fewer than 'min loaded tabs' will remain loaded if possible.
// The 'max loaded tabs' acts as a soft cap on the number of tabs that remain
// loaded, and in the absence of user activity the number of loaded tabs will
// always trend to that number. The 'min occluded timeout' acts as a short term
// protection which can cause the instantaneous total number of loaded tabs to
// exceed the cap.
//
// This logic is indepdendent of urgent discarding, which may embark when things
// are sufficiently bad and violate the invariant. Similarly, manual or
// extension driven discards can override this logic.
//
// NOTE: This is extremely simplistic, and by design. We will be using this to
// do a very simple "lightspeed" experiment to determine how much possible
// savings proactive discarding can hope to achieve.
struct ProactiveTabDiscardParams {
  // The number of tabs to allow Chrome to use per GB of system memory before
  // proactive discarding starts kicking in. This is a very simple cap that
  // depends on actual system resources.
  int max_loaded_tabs_per_gb_ram;
  // The minimum number of tabs that have to be loaded before proactive
  // discarding can start discarding. This is independent of system resources,
  // and is intended to be conservative enough that average users would be
  // unable to reach severe memory pressure with this many tabs open. Urgent tab
  // killing will still be invoked if memory pressure is encountered.
  int min_loaded_tab_count;
  // The maximum number of tabs that are allowed to be loaded at any point in
  // time. This is a hard cap across a browser instance. Tabs will be
  // immediately discarded upon a tab switch / tab creation once this number is
  // exceeded, regardless of the amount of time a tab was inactive. This cap is
  // independent of system resources, and is intended to be quite generous.
  // Urgent tab killing could still cause a tab to be discarded prior to this.
  int max_loaded_tab_count;
  // The amount of time a tab must be occluded (foreground tab but minimized,
  // foreground tab but covered by other windows, or backgrounded) before it is
  // eligible for proactive discarding. Urgent tab killing could still cause a
  // tab to be discarded prior to this.
  base::TimeDelta min_occluded_timeout;
  // The amount of time a tab must be occluded (foreground tab but minimized,
  // foreground tab but covered by other windows, or backgrounded) before it
  // will be automatically proactively discarded, regardless of the number of
  // tabs.
  base::TimeDelta max_occluded_timeout;
};

// Gets parameters for the proactive tab discarding feature. This does no
// parameter validation.
void GetProactiveTabDiscardParams(ProactiveTabDiscardParams* params);

base::TimeDelta GetTabLoadTimeout(const base::TimeDelta& default_timeout);

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_FEATURES_H_
