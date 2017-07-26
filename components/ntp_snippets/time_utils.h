// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_TIME_UTILS_H_
#define COMPONENTS_NTP_SNIPPETS_TIME_UTILS_H_

#include "base/time/time.h"

namespace ntp_snippets {

// Backward compatible replacements for deprecated
// base::Time::To/FromInternalValue.
int64_t SerializeTime(const base::Time& time);
base::Time DeserializeTime(int64_t serialized_time);

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_TIME_UTILS_H_
