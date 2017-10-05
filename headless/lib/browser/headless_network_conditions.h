// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_CONDITIONS_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_CONDITIONS_H_

#include <string>
#include <vector>

#include "base/macros.h"

#include "headless/public/headless_export.h"

namespace headless {

// HeadlessNetworkConditions holds information about desired network conditions.
class HEADLESS_EXPORT HeadlessNetworkConditions {
 public:
  HeadlessNetworkConditions();
  ~HeadlessNetworkConditions();

  explicit HeadlessNetworkConditions(bool offline);
  HeadlessNetworkConditions(bool offline,
                            double latency,
                            double download_throughput,
                            double upload_throughput);

  bool IsThrottling() const;

  bool offline() const { return offline_; }
  double latency() const { return latency_; }
  double download_throughput() const { return download_throughput_; }
  double upload_throughput() const { return upload_throughput_; }

 private:
  const bool offline_;
  const double latency_;
  const double download_throughput_;
  const double upload_throughput_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessNetworkConditions);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_CONDITIONS_H_
