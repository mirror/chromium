/**
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/css/CSSProperty.h"

#include "core/css/properties/CSSPropertyAPI.h"
#include "core/style/ComputedStyleConstants.h"

namespace blink {

struct SameSizeAsCSSProperty {
  uint32_t bitfields;
  Member<void*> value;
};

static_assert(sizeof(CSSProperty) == sizeof(SameSizeAsCSSProperty),
              "CSSProperty should stay small");

CSSPropertyID StylePropertyMetadata::ShorthandID() const {
  if (!is_set_from_shorthand_)
    return CSSPropertyInvalid;

  Vector<StylePropertyShorthand, 4> shorthands;
  getMatchingShorthandsForLonghand(static_cast<CSSPropertyID>(property_id_),
                                   &shorthands);
  DCHECK(shorthands.size());
  DCHECK_GE(index_in_shorthands_vector_, 0u);
  DCHECK_LT(index_in_shorthands_vector_, shorthands.size());
  return shorthands.at(index_in_shorthands_vector_).id();
}

enum PhysicalBoxSide { kTopSide, kRightSide, kBottomSide, kLeftSide };

static CSSPropertyID ResolveToPhysicalProperty(
    TextDirection direction,
    WritingMode writing_mode,
    LogicalBoxSide logical_side,
    const StylePropertyShorthand& shorthand) {
  if (direction == TextDirection::kLtr) {
    if (IsHorizontalWritingMode(writing_mode)) {
      // The common case. The logical and physical box sides match.
      // Left = Start, Right = End, Before = Top, After = Bottom
      return shorthand.properties()[logical_side];
    }

    if (IsFlippedLinesWritingMode(writing_mode)) {
      // Start = Top, End = Bottom, Before = Left, After = Right.
      switch (logical_side) {
        case kStartSide:
          return shorthand.properties()[kTopSide];
        case kEndSide:
          return shorthand.properties()[kBottomSide];
        case kBeforeSide:
          return shorthand.properties()[kLeftSide];
        default:
          return shorthand.properties()[kRightSide];
      }
    }

    // Start = Top, End = Bottom, Before = Right, After = Left
    switch (logical_side) {
      case kStartSide:
        return shorthand.properties()[kTopSide];
      case kEndSide:
        return shorthand.properties()[kBottomSide];
      case kBeforeSide:
        return shorthand.properties()[kRightSide];
      default:
        return shorthand.properties()[kLeftSide];
    }
  }

  if (IsHorizontalWritingMode(writing_mode)) {
    // Start = Right, End = Left, Before = Top, After = Bottom
    switch (logical_side) {
      case kStartSide:
        return shorthand.properties()[kRightSide];
      case kEndSide:
        return shorthand.properties()[kLeftSide];
      case kBeforeSide:
        return shorthand.properties()[kTopSide];
      default:
        return shorthand.properties()[kBottomSide];
    }
  }

  if (IsFlippedLinesWritingMode(writing_mode)) {
    // Start = Bottom, End = Top, Before = Left, After = Right
    switch (logical_side) {
      case kStartSide:
        return shorthand.properties()[kBottomSide];
      case kEndSide:
        return shorthand.properties()[kTopSide];
      case kBeforeSide:
        return shorthand.properties()[kLeftSide];
      default:
        return shorthand.properties()[kRightSide];
    }
  }

  // Start = Bottom, End = Top, Before = Right, After = Left
  switch (logical_side) {
    case kStartSide:
      return shorthand.properties()[kBottomSide];
    case kEndSide:
      return shorthand.properties()[kTopSide];
    case kBeforeSide:
      return shorthand.properties()[kRightSide];
    default:
      return shorthand.properties()[kLeftSide];
  }
}

const CSSPropertyAPI& CSSProperty::ResolveToPhysicalPropertyAPI(
    TextDirection direction,
    WritingMode writing_mode,
    LogicalBoxSide logical_side,
    const StylePropertyShorthand& shorthand) {
  CSSPropertyID id = ResolveToPhysicalProperty(direction, writing_mode,
                                               logical_side, shorthand);
  return CSSPropertyAPI::Get(id);
}

static CSSPropertyID ResolveToPhysicalProperty(
    WritingMode writing_mode,
    LogicalExtent logical_side,
    const CSSPropertyID* properties) {
  if (IsHorizontalWritingMode(writing_mode))
    return properties[logical_side];
  return logical_side == kLogicalWidth ? properties[1] : properties[0];
}

const CSSPropertyAPI& CSSProperty::ResolveToPhysicalPropertyAPI(
    WritingMode writing_mode,
    LogicalExtent logical_side,
    const CSSPropertyID* properties) {
  CSSPropertyID id =
      ResolveToPhysicalProperty(writing_mode, logical_side, properties);
  return CSSPropertyAPI::Get(id);
}

const StylePropertyShorthand& CSSProperty::BorderDirections() {
  static const CSSPropertyID kProperties[4] = {
      CSSPropertyBorderTop, CSSPropertyBorderRight, CSSPropertyBorderBottom,
      CSSPropertyBorderLeft};
  DEFINE_STATIC_LOCAL(
      StylePropertyShorthand, border_directions,
      (CSSPropertyBorder, kProperties, WTF_ARRAY_LENGTH(kProperties)));
  return border_directions;
}

void CSSProperty::FilterEnabledCSSPropertiesIntoVector(
    const CSSPropertyID* properties,
    size_t propertyCount,
    Vector<CSSPropertyID>& outVector) {
  for (unsigned i = 0; i < propertyCount; i++) {
    CSSPropertyID property = properties[i];
    if (CSSPropertyAPI::Get(property).IsEnabled())
      outVector.push_back(property);
  }
}

bool CSSProperty::operator==(const CSSProperty& other) const {
  return DataEquivalent(value_, other.value_) &&
         IsImportant() == other.IsImportant();
}

}  // namespace blink
