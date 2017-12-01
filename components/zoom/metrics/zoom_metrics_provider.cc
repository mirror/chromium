// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/metrics/zoom_metrics_provider.h"

#include <vector>

#include "components/zoom/metrics/style_info.h"
#include "components/zoom/metrics/zoom_metrics_cache.h"
#include "third_party/metrics_proto/chrome_user_metrics_extension.pb.h"
#include "third_party/metrics_proto/zoom_event.pb.h"

namespace zoom {
namespace metrics {

ZoomMetricsProvider::ZoomMetricsProvider() {}

ZoomMetricsProvider::~ZoomMetricsProvider() {}

void ZoomMetricsProvider::ProvideCurrentSessionData(
    ::metrics::ChromeUserMetricsExtension* uma_proto) {
  std::vector<::metrics::ZoomEventProto> zoom_events;
  ZoomMetricsCache::GetInstance()->FlushZoomEvents(&zoom_events);

  for (auto& event : zoom_events)
    uma_proto->add_zoom_event()->Swap(&event);
}

void ZoomMetricsProvider::OnRecordingEnabled() {
  ZoomMetricsCache::GetInstance()->OnRecordingEnabled();
}

void ZoomMetricsProvider::OnRecordingDisabled() {
  ZoomMetricsCache::GetInstance()->OnRecordingDisabled();
}

}  // namespace metrics
}  // namespace zoom
