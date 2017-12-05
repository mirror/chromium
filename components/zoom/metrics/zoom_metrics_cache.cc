// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/metrics/zoom_metrics_cache.h"

#include "base/memory/singleton.h"
#include "components/zoom/metrics/style_info.h"
#include "third_party/metrics_proto/zoom_event.pb.h"

namespace zoom {
namespace metrics {

ZoomMetricsCache* ZoomMetricsCache::GetInstance() {
  return base::Singleton<ZoomMetricsCache>::get();
}

ZoomMetricsCache::ZoomMetricsCache() : recording_enabled_(false) {}

ZoomMetricsCache::~ZoomMetricsCache() {}

void ZoomMetricsCache::AddZoomEvent(float zoom_factor,
                                    float default_zoom_factor,
                                    const StyleInfo& style_info) {
  if (!recording_enabled_)
    return;

  ::metrics::ZoomEventProto zoom_event;

  DCHECK(style_info.main_frame_info);
  const StyleInfo::MainFrameStyleInfo& main_info =
      style_info.main_frame_info.value();

  zoom_event.set_zoom_factor(zoom_factor);
  zoom_event.set_default_zoom_factor(default_zoom_factor);
  zoom_event.set_visible_content_width(main_info.visible_content_size.width());
  zoom_event.set_visible_content_height(
      main_info.visible_content_size.height());
  zoom_event.set_contents_width(main_info.contents_size.width());
  zoom_event.set_contents_height(main_info.contents_size.height());
  zoom_event.set_preferred_width(main_info.preferred_width);

  uint32_t total_characters = 0;
  for (const auto& font : style_info.font_size_distribution) {
    total_characters += font.second;
  }

  // There is no need for the raw character counts, so we'll only send the font
  // size usage as a percentage.
  if (total_characters > 0) {
    auto* font_percentages = zoom_event.mutable_font_size_distribution();
    for (const auto& font : style_info.font_size_distribution) {
      font_percentages->insert(google::protobuf::MapPair<uint32_t, float>(
          font.first, static_cast<float>(font.second) / total_characters));
    }
  }

  if (style_info.largest_image) {
    zoom_event.set_largest_image_width(style_info.largest_image->width());
    zoom_event.set_largest_image_height(style_info.largest_image->height());
  }

  if (style_info.largest_object) {
    zoom_event.set_largest_object_width(style_info.largest_object->width());
    zoom_event.set_largest_object_height(style_info.largest_object->height());
  }

  zoom_event.set_text_area(style_info.text_area);
  zoom_event.set_image_area(style_info.image_area);
  zoom_event.set_object_area(style_info.object_area);

  event_cache_.push_back(zoom_event);
}

void ZoomMetricsCache::FlushZoomEvents(
    std::vector<::metrics::ZoomEventProto>* events) {
  events->swap(event_cache_);
  event_cache_.clear();
}

bool ZoomMetricsCache::IsRecordingEnabled() {
  return recording_enabled_;
}

void ZoomMetricsCache::OnRecordingEnabled() {
  recording_enabled_ = true;
}

void ZoomMetricsCache::OnRecordingDisabled() {
  recording_enabled_ = false;
  event_cache_.clear();
}

}  // namespace metrics
}  // namespace zoom
