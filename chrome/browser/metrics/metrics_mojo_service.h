// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_METRICS_MOJO_SERVICE_H_
#define CHROME_BROWSER_METRICS_METRICS_MOJO_SERVICE_H_

#include <memory>

#include "services/service_manager/public/cpp/service.h"

namespace metrics {

std::unique_ptr<service_manager::Service> CreateMetricsService();

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_METRICS_MOJO_SERVICE_H_
