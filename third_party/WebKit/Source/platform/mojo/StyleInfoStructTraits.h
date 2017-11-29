// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StyleInfoStructTraits_h
#define StyleInfoStructTraits_h

#include "platform/wtf/HashMap.h"
#include "platform/wtf/Optional.h"
#include "platform/zoom_metrics/StyleInfo.h"
#include "public/platform/WebSize.h"
#include "public/platform/modules/zoom_metrics/style_collector.mojom-blink.h"

namespace mojo {

template <>
struct StructTraits<
    ::blink::mojom::zoom_metrics::blink::MainFrameStyleInfo::DataView,
    ::blink::zoom_metrics::StyleInfo::MainFrameStyleInfo> {
  static blink::WebSize visible_content_size(
      const ::blink::zoom_metrics::StyleInfo::MainFrameStyleInfo& info) {
    return info.visible_content_size;
  }

  static blink::WebSize contents_size(
      const ::blink::zoom_metrics::StyleInfo::MainFrameStyleInfo& info) {
    return info.contents_size;
  }

  static uint32_t preferred_width(
      const ::blink::zoom_metrics::StyleInfo::MainFrameStyleInfo& info) {
    return info.preferred_width;
  }

  static bool Read(
      ::blink::mojom::zoom_metrics::blink::MainFrameStyleInfo::DataView data,
      ::blink::zoom_metrics::StyleInfo::MainFrameStyleInfo* out) {
    if (!data.ReadVisibleContentSize(&out->visible_content_size))
      return false;

    if (!data.ReadContentsSize(&out->contents_size))
      return false;

    out->preferred_width = data.preferred_width();
    return true;
  }
};

template <>
struct StructTraits<::blink::mojom::zoom_metrics::blink::StyleInfo::DataView,
                    ::blink::zoom_metrics::StyleInfo> {
  static WTF::HashMap<uint8_t, uint32_t> font_size_distribution(
      const ::blink::zoom_metrics::StyleInfo& info) {
    return info.font_size_distribution;
  }

  static WTF::Optional<::blink::zoom_metrics::StyleInfo::MainFrameStyleInfo>
  main_frame_info(const ::blink::zoom_metrics::StyleInfo& info) {
    return info.main_frame_info;
  }

  static WTF::Optional<blink::WebSize> largest_image(
      const ::blink::zoom_metrics::StyleInfo& info) {
    return info.largest_image;
  }

  static WTF::Optional<blink::WebSize> largest_object(
      const ::blink::zoom_metrics::StyleInfo& info) {
    return info.largest_object;
  }

  static uint32_t text_area(const ::blink::zoom_metrics::StyleInfo& info) {
    return info.text_area;
  }

  static uint32_t image_area(const ::blink::zoom_metrics::StyleInfo& info) {
    return info.image_area;
  }

  static uint32_t object_area(const ::blink::zoom_metrics::StyleInfo& info) {
    return info.object_area;
  }

  static bool Read(
      ::blink::mojom::zoom_metrics::blink::StyleInfo::DataView data,
      ::blink::zoom_metrics::StyleInfo* out) {
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

#endif  // StyleInfoStructTraits_h
