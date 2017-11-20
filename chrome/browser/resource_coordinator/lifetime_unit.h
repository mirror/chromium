// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_H_

#include "base/containers/flat_set.h"
#include "base/process/process_handle.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/resource_coordinator/discard_condition.h"

namespace resource_coordinator {

class TabLifetimeUnit;

// A LifetimeUnit represents a unit that can switch between the "alive" and
// "discarded" states. When it is alive, the unit uses system resources and
// provides functionality to the user. When it is discarded, the unit doesn't
// use any system resource.
class LifetimeUnit {
 public:
  enum class State {
    // The LifetimeUnit is using system resources.
    ALIVE,
    // The LifetimeUnit is not using system resources.
    DISCARDED,
  };

  // Used to sort LifetimeUnit by importance. The most important LifetimeUnit
  // has the greatest SortKey.
  struct SortKey {
    SortKey();
    SortKey(base::TimeTicks last_focused_time);

    bool operator<(const SortKey& other) const;
    bool operator>(const SortKey& other) const;

    // Last time at which the LifetimeUnit was focused. base::TimeTicks::Max()
    // if the LifetimeUnit is currently focused.
    base::TimeTicks last_focused_time;
  };

  virtual ~LifetimeUnit() = default;

  // Casts |this| to a TabLifetimeUnit if possible.
  // TODO(fdoray): Remove this method once the purge logic has been moved out of
  // TabManager. https://crbug.com/775644
  virtual TabLifetimeUnit* AsTabLifetimeUnit() = 0;

  // Returns a title describing this LifetimeUnit.
  virtual base::string16 GetTitle() const = 0;

#if defined(OS_CHROMEOS)
  // Returns the handle of the process in which this LifetimeUnit lives.
  virtual base::ProcessHandle GetProcessHandle() const = 0;
#endif  // defined(OS_CHROMEOS)

  // Returns a key that can be used to evaluate the relative importance of this
  // LifetimeUnit.
  virtual SortKey GetSortKey() const = 0;

  // Returns the current state of this LifetimeUnit.
  virtual State GetState() const = 0;

  // Returns the estimated number of kilobytes that would be freed if this
  // LifetimeUnit was discarded.
  virtual int GetEstimatedMemoryFreedOnDiscardKB() const = 0;

  // Returns true if this LifetimeUnit can be discared.
  virtual bool CanDiscard(DiscardCondition discard_condition) const = 0;

  // Discards this LifetimeUnit.
  virtual bool Discard(DiscardCondition discard_condition) = 0;
};

using LifetimeUnitSet = base::flat_set<LifetimeUnit*>;

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_H_
