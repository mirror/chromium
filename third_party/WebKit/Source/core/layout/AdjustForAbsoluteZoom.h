// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AdjustForAbsoluteZoom_h
#define AdjustForAbsoluteZoom_h

#include "core/layout/LayoutObject.h"
#include "core/style/ComputedStyle.h"
#include "platform/LayoutUnit.h"

namespace blink {

class AdjustForAbsoluteZoom {
  STATIC_ONLY(AdjustForAbsoluteZoom);

 public:
  // FIXME: Reduce/remove the dependency on zoom adjusted int values.
  // The float or LayoutUnit versions of layout values should be used.
  static int AdjustInt(int value, float zoom_factor) {
    if (zoom_factor == 1)
      return value;
    // Needed because computeLengthInt truncates (rather than rounds) when
    // scaling up.
    float fvalue = value;
    if (zoom_factor > 1) {
      if (value < 0)
        fvalue -= 0.5f;
      else
        fvalue += 0.5f;
    }
    return RoundForImpreciseConversion<int>(fvalue / zoom_factor);
  }
  inline static int AdjustInt(int value, const ComputedStyle& style) {
    float zoom_factor = style.EffectiveZoom();
    if (zoom_factor == 1)
      return value;
    return AdjustInt(value, zoom_factor);
  }
  inline static int AdjustInt(int value, LayoutObject* layout_object) {
    DCHECK(layout_object);
    return AdjustInt(value, layout_object->StyleRef());
  }

  inline static float AdjustFloat(float value, const ComputedStyle& style) {
    return value / style.EffectiveZoom();
  }

  inline static double AdjustDouble(double value, const ComputedStyle& style) {
    return value / style.EffectiveZoom();
  }

  inline static LayoutUnit AdjustLayoutUnit(LayoutUnit value,
                                            const ComputedStyle& style) {
    return LayoutUnit(value / style.EffectiveZoom());
  }
  inline static LayoutUnit AdjustLayoutUnit(LayoutUnit value,
                                            LayoutObject& layout_object) {
    return AdjustLayoutUnit(value, layout_object.StyleRef());
  }

  inline static void AdjustFloatQuad(FloatQuad& quad,
                                     const LayoutObject& layout_object) {
    float zoom = layout_object.StyleRef().EffectiveZoom();
    if (zoom != 1)
      quad.Scale(1 / zoom, 1 / zoom);
  }
  inline static void AdjustFloatRect(FloatRect& rect,
                                     const LayoutObject& layout_object) {
    float zoom = layout_object.StyleRef().EffectiveZoom();
    if (zoom != 1)
      rect.Scale(1 / zoom, 1 / zoom);
  }

  inline static float AdjustScroll(float scroll_offset, float zoom_factor) {
    return scroll_offset / zoom_factor;
  }
  inline static float AdjustScroll(float scroll_offset,
                                   const ComputedStyle& style) {
    return AdjustScroll(scroll_offset, style.EffectiveZoom());
  }
  inline static double AdjustScroll(double value, LayoutObject& layout_object) {
    return AdjustScroll(value, layout_object.StyleRef());
  }
};

}  // namespace blink

#endif  // AdjustForAbsoluteZoom_h
