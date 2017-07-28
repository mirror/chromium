// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_METRICS_USER_METRICS_ACTION_H_
#define BASE_METRICS_USER_METRICS_ACTION_H_

namespace base {

// UserMetricsAction exists purely to standardize on the parameters passed to
// UserMetrics. That way, our toolset can scan the source code reliable for
// constructors and extract the associated string constants.
// Please see tools/metrics/actions/extract_actions.py for details.
struct UserMetricsAction {
  const char* const str_;
  template <size_t N>
  explicit UserMetricsAction(const char (&str)[N]) : str_(str) {}
};

}  // namespace base

#endif  // BASE_METRICS_USER_METRICS_ACTION_H_
