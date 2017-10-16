// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARDABLE_UNIT_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARDABLE_UNIT_H_

#include "chrome/browser/resource_coordinator/system_condition.h"

namespace resource_coordinator {

class DiscardableUnit {
 public:
  enum class State {
    NEVER_LOADED,
    ALIVE,
    DISCARDED,
  };

  // Returns the current state of the DiscardableUnit.
  virtual State GetState() const = 0;

  // Returns true if this DiscardableUnit can be discarded.
  virtual bool CanDiscard(SystemCondition system_condition) const = 0;

  virtual void Discard(State state, SystemCondition system_condition) = 0;

  virtual void Load() = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARDABLE_UNIT_H_
