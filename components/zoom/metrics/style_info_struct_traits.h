// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZOOM_METRICS_STYLE_INFO_STRUCT_TRAITS_H_
#define COMPONENTS_ZOOM_METRICS_STYLE_INFO_STRUCT_TRAITS_H_

#include <map>

#include "base/optional.h"
#include "components/zoom/metrics/style_info.h"
#include "third_party/WebKit/public/platform/modules/zoom_metrics/style_collector.mojom.h"
#include "ui/gfx/geometry/size.h"

namespace mojo {

template <>
struct StructTraits<::blink::mojom::zoom_metrics::MainFrameStyleInfo::DataView,
                    ::zoom::metrics::StyleInfo::MainFrameStyleInfo> {
  static gfx::Size visible_content_size(
      const ::zoom::metrics::StyleInfo::MainFrameStyleInfo& info) {
    return info.visible_content_size;
  }

  static gfx::Size contents_size(
      const ::zoom::metrics::StyleInfo::MainFrameStyleInfo& info) {
    return info.contents_size;
  }

  static uint32_t preferred_width(
      const ::zoom::metrics::StyleInfo::MainFrameStyleInfo& info) {
    return info.preferred_width;
  }

  static bool Read(
      ::blink::mojom::zoom_metrics::MainFrameStyleInfo::DataView data,
      ::zoom::metrics::StyleInfo::MainFrameStyleInfo* out) {
    if (!data.ReadVisibleContentSize(&out->visible_content_size))
      return false;

    if (!data.ReadContentsSize(&out->contents_size))
      return false;

    out->preferred_width = data.preferred_width();
    return true;
  }
};

template <>
struct StructTraits<::blink::mojom::zoom_metrics::StyleInfo::DataView,
                    ::zoom::metrics::StyleInfo> {
  static std::map<uint8_t, uint32_t> font_size_distribution(
      const ::zoom::metrics::StyleInfo& info) {
    return info.font_size_distribution;
  }

  static base::Optional<::zoom::metrics::StyleInfo::MainFrameStyleInfo>
  main_frame_info(const ::zoom::metrics::StyleInfo& info) {
    return info.main_frame_info;
  }

  static base::Optional<gfx::Size> largest_image(
      const ::zoom::metrics::StyleInfo& info) {
    return info.largest_image;
  }

  static base::Optional<gfx::Size> largest_object(
      const ::zoom::metrics::StyleInfo& info) {
    return info.largest_object;
  }

  static uint32_t text_area(const ::zoom::metrics::StyleInfo& info) {
    return info.text_area;
  }

  static uint32_t image_area(const ::zoom::metrics::StyleInfo& info) {
    return info.image_area;
  }

  static uint32_t object_area(const ::zoom::metrics::StyleInfo& info) {
    return info.object_area;
  }

  static bool Read(::blink::mojom::zoom_metrics::StyleInfo::DataView data,
                   ::zoom::metrics::StyleInfo* out) {
    if (!data.ReadFontSizeDistribution(&out->font_size_distribution))
      return false;

    if (!data.ReadMainFrameInfo(&out->main_frame_info))
      return false;

    if (!data.ReadLargestImage(&out->largest_image))
      return false;

    if (!data.ReadLargestObject(&out->largest_object))
      return false;

    out->text_area = data.text_area();
    out->image_area = data.image_area();
    out->object_area = data.object_area();
    return true;
  }
};

}  // namespace mojo

#endif  // COMPONENTS_ZOOM_METRICS_STYLE_INFO_STRUCT_TRAITS_H_
