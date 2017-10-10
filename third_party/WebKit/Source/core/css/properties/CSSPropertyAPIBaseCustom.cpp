// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains methods of CSSPropertyAPI that are not generated.

#include "core/css/properties/CSSPropertyAPI.h"

#include "core/StylePropertyShorthand.h"

namespace blink {

const StylePropertyShorthand& CSSPropertyAPI::BorderDirections() {
  static const CSSPropertyID kProperties[4] = {
      CSSPropertyBorderTop, CSSPropertyBorderRight, CSSPropertyBorderBottom,
      CSSPropertyBorderLeft};
  DEFINE_STATIC_LOCAL(
      StylePropertyShorthand, border_directions,
      (CSSPropertyBorder, kProperties, WTF_ARRAY_LENGTH(kProperties)));
  return border_directions;
}

const CSSPropertyAPI& CSSPropertyAPI::ResolveAfterToPhysicalPropertyAPI(
    TextDirection direction,
    WritingMode writing_mode,
    const StylePropertyShorthand& shorthand) {
  const CSSPropertyID* shorthand_properties = shorthand.properties();
  if (IsHorizontalWritingMode(writing_mode))
    return Get(shorthand_properties[kBottomSide]);
  if (IsFlippedLinesWritingMode(writing_mode))
    return Get(shorthand_properties[kRightSide]);
  return Get(shorthand_properties[kLeftSide]);
}

const CSSPropertyAPI& CSSPropertyAPI::ResolveBeforeToPhysicalPropertyAPI(
    TextDirection direction,
    WritingMode writing_mode,
    const StylePropertyShorthand& shorthand) {
  const CSSPropertyID* shorthand_properties = shorthand.properties();
  if (IsHorizontalWritingMode(writing_mode))
    return Get(shorthand_properties[kTopSide]);
  if (IsFlippedLinesWritingMode(writing_mode))
    return Get(shorthand_properties[kLeftSide]);
  return Get(shorthand_properties[kRightSide]);
}

const CSSPropertyAPI& CSSPropertyAPI::ResolveEndToPhysicalPropertyAPI(
    TextDirection direction,
    WritingMode writing_mode,
    const StylePropertyShorthand& shorthand) {
  const CSSPropertyID* shorthand_properties = shorthand.properties();
  if (direction == TextDirection::kLtr) {
    if (IsHorizontalWritingMode(writing_mode))
      return Get(shorthand_properties[kRightSide]);
    return Get(shorthand_properties[kBottomSide]);
  }
  if (IsHorizontalWritingMode(writing_mode))
    return Get(shorthand_properties[kLeftSide]);
  return Get(shorthand_properties[kTopSide]);
}

const CSSPropertyAPI& CSSPropertyAPI::ResolveStartToPhysicalPropertyAPI(
    TextDirection direction,
    WritingMode writing_mode,
    const StylePropertyShorthand& shorthand) {
  const CSSPropertyID* shorthand_properties = shorthand.properties();
  if (direction == TextDirection::kLtr) {
    if (IsHorizontalWritingMode(writing_mode))
      return Get(shorthand_properties[kLeftSide]);
    return Get(shorthand_properties[kTopSide]);
  }
  if (IsHorizontalWritingMode(writing_mode))
    return Get(shorthand_properties[kRightSide]);
  return Get(shorthand_properties[kBottomSide]);
}

const CSSPropertyAPI& CSSPropertyAPI::ResolveToPhysicalPropertyAPI(
    WritingMode writing_mode,
    LogicalExtent logical_side,
    const CSSPropertyID* properties) {
  if (IsHorizontalWritingMode(writing_mode))
    return Get(properties[logical_side]);
  CSSPropertyID id =
      logical_side == kLogicalWidth ? properties[1] : properties[0];
  return Get(id);
}

}  // namespace blink
