// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_CACHE_H_
#define COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_CACHE_H_

#include <vector>

#include "base/macros.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace metrics {
class ZoomEventProto;
}  // namespace metrics

namespace zoom {
namespace metrics {

struct StyleInfo;

// Store zoom metrics events until ZoomMetricsProvider flushes them.
class ZoomMetricsCache {
 public:
  ~ZoomMetricsCache();

  static ZoomMetricsCache* GetInstance();

  // Creates a zoom event proto and stores it until it is offered to
  // ZoomMetricsProvider.
  // If recording is disabled, the event is dropped.
  void AddZoomEvent(float zoom_factor,
                    float default_zoom_factor,
                    const StyleInfo& style_info);

  // Called by ZoomMetricsProvider to provide the stored zoom event protos to
  // the metrics service.
  void FlushZoomEvents(std::vector<::metrics::ZoomEventProto>* events);

  bool IsRecordingEnabled();

  void OnRecordingEnabled();
  void OnRecordingDisabled();

 private:
  ZoomMetricsCache();
  friend struct base::DefaultSingletonTraits<ZoomMetricsCache>;

  // Whether recording is enabled by the metrics service.
  bool recording_enabled_;

  // Cache of zoom event protos.
  std::vector<::metrics::ZoomEventProto> event_cache_;

  DISALLOW_COPY_AND_ASSIGN(ZoomMetricsCache);
};

}  // namespace metrics
}  // namespace zoom

#endif  // COMPONENTS_ZOOM_METRICS_ZOOM_METRICS_CACHE_H_
