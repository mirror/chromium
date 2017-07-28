// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_TIME_UTILS_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_TIME_UTILS_H_

#include <stdint.h>

namespace base {
class Time;
}

namespace offline_pages {

// Style compliant serialization and de-serialization methods for time data to
// ease conversions to and from stored data.
int64_t SerializeTime(base::Time time);
base::Time DeserializeTime(int64_t serialized_time);

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_TIME_UTILS_H_
