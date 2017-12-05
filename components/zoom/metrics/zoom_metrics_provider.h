// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_PROVIDER_H_
#define COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_PROVIDER_H_

#include "base/macros.h"
#include "components/metrics/metrics_provider.h"

namespace zoom {
namespace metrics {

// Provides metrics related to the use of browser zoom.
class ZoomMetricsProvider : public ::metrics::MetricsProvider {
 public:
  ZoomMetricsProvider();
  ~ZoomMetricsProvider() override;

  // metrics::MetricsProvider:
  void ProvideCurrentSessionData(
      ::metrics::ChromeUserMetricsExtension* uma_proto) override;
  void OnRecordingEnabled() override;
  void OnRecordingDisabled() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ZoomMetricsProvider);
};

}  // namespace metrics
}  // namespace zoom

#endif  // COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_PROVIDER_H_
