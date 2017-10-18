// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARD_MANAGER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARD_MANAGER_H_

#include "base/macros.h"

namespace resource_coordinator {

class DiscardableUnit;

class DiscardManager {
 public:
  DiscardManager();
  ~DiscardManager();

  void OnDiscardableUnitCreated(DiscardableUnit* discardable_unit);
  void OnDiscardableUnitDestroyed(DiscardableUnit* discardable_unit);
  void OnDiscardableUnitFocused(DiscardableUnit* discardable_unit);
  void OnDiscardableUnitVisibilityChanged(DiscardableUnit* discardable_unit);
  void OnDiscardableUnitLoaded(DiscardableUnit* discardable_unit);

 private:
  DISALLOW_COPY_AND_ASSIGN(DiscardManager);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARDABLE_UNIT_H_
