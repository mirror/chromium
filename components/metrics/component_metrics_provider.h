// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_COMPONENT_METRICS_PROVIDER_H_
#define COMPONENTS_METRICS_COMPONENT_METRICS_PROVIDER_H_

#include "components/metrics/metrics_provider.h"

namespace component_updater {

class ComponentUpdateService;

}  // namespace component_updater

namespace metrics {

class SystemProfileProto;

// Stores and loads system information to prefs for stability logs.
class ComponentMetricsProvider : public MetricsProvider {
 public:
  ComponentMetricsProvider(component_updater::ComponentUpdateService* cus);
  ~ComponentMetricsProvider() override;

  // MetricsProvider
  void ProvideSystemProfileMetrics(
      SystemProfileProto* system_profile_proto) override;

 private:
  component_updater::ComponentUpdateService* cus_;

  DISALLOW_COPY_AND_ASSIGN(ComponentMetricsProvider);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_COMPONENT_METRICS_PROVIDER_H_
