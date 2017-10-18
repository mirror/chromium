// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_SYSTEM_CONDITION_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_SYSTEM_CONDITION_H_

namespace resource_coordinator {

enum class SystemCondition {
  NORMAL,
  CRITICAL_MEMORY_PRESSURE,
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_SYSTEM_CONDITION_H_
