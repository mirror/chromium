// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebScrollIntoViewParams.h"

#include "platform/wtf/SizeAssertions.h"

namespace blink {
namespace {
using Alignment = WebScrollIntoViewParams::Alignment;
using Type = WebScrollIntoViewParams::Type;
using Behavior = WebScrollIntoViewParams::Behavior;

// Make sure we keep the public enums in sync with the internal ones.

ASSERT_SIZE(Type, ScrollType)
STATIC_ASSERT_ENUM(Type::kUser, ScrollType::kUserScroll);
STATIC_ASSERT_ENUM(Type::kProgrammatic, ScrollType::kProgrammaticScroll);
STATIC_ASSERT_ENUM(Type::kClamping, ScrollType::kClampingScroll);
STATIC_ASSERT_ENUM(Type::kAnchoring, ScrollType::kAnchoringScroll);
STATIC_ASSERT_ENUM(Type::kSequenced, ScrollType::kSequencedScroll);

ASSERT_SIZE(Behavior, ScrollBehavior)
STATIC_ASSERT_ENUM(Behavior::kAuto, ScrollBehavior::kScrollBehaviorAuto);
STATIC_ASSERT_ENUM(Behavior::kInstant, ScrollBehavior::kScrollBehaviorInstant);
STATIC_ASSERT_ENUM(Behavior::kSmooth, ScrollBehavior::kScrollBehaviorSmooth);

Alignment ToAlignment(const ScrollAlignment& alignment) {
  if (alignment == ScrollAlignment::kAlignCenterIfNeeded)
    return Alignment::kCenterIfNeeded;
  if (alignment == ScrollAlignment::kAlignToEdgeIfNeeded)
    return Alignment::kToEdgeIfNeeded;
  if (alignment == ScrollAlignment::kAlignCenterAlways)
    return Alignment::kCenterAlways;
  if (alignment == ScrollAlignment::kAlignTopAlways)
    return Alignment::kTopAlways;
  if (alignment == ScrollAlignment::kAlignBottomAlways)
    return Alignment::kBottomAlways;
  if (alignment == ScrollAlignment::kAlignLeftAlways)
    return Alignment::kLeftAlways;
  if (alignment == ScrollAlignment::kAlignRightAlways)
    return Alignment::kRightAlways;
  NOTREACHED();
  return Alignment::kCenterIfNeeded;
}

ScrollAlignment ToScrollAlignment(Alignment alignment) {
  switch (alignment) {
    case Alignment::kCenterIfNeeded:
      return ScrollAlignment::kAlignCenterIfNeeded;
    case Alignment::kToEdgeIfNeeded:
      return ScrollAlignment::kAlignToEdgeIfNeeded;
    case Alignment::kCenterAlways:
      return ScrollAlignment::kAlignCenterAlways;
    case Alignment::kTopAlways:
      return ScrollAlignment::kAlignTopAlways;
    case Alignment::kBottomAlways:
      return ScrollAlignment::kAlignBottomAlways;
    case Alignment::kLeftAlways:
      return ScrollAlignment::kAlignLeftAlways;
    case Alignment::kRightAlways:
      return ScrollAlignment::kAlignRightAlways;
    default:
      NOTREACHED();
      return ScrollAlignment::kAlignCenterIfNeeded;
  }
}

Type FromScrollType(ScrollType type) {
  return static_cast<Type>(static_cast<int>(type));
}

ScrollType ToScrollType(Type type) {
  return static_cast<ScrollType>(static_cast<int>(type));
}

Behavior FromScrollBehavior(ScrollBehavior behavior) {
  return static_cast<Behavior>(static_cast<int>(behavior));
}

ScrollBehavior ToScrollBehavior(Behavior behavior) {
  return static_cast<ScrollBehavior>(static_cast<int>(behavior));
}

}  // namespace

WebScrollIntoViewParams::WebScrollIntoViewParams(
    const ScrollIntoViewParams& params)
    : align_x(ToAlignment(params.align_x)),
      align_y(ToAlignment(params.align_y)),
      type(FromScrollType(params.scroll_type)),
      make_visible_in_visual_viewport(params.make_visible_in_visual_viewport),
      behavior(FromScrollBehavior(params.scroll_behavior)),
      is_for_scroll_sequence(params.is_for_scroll_sequence) {}

WebScrollIntoViewParams::operator ScrollIntoViewParams() const {
  return {ToScrollAlignment(align_x), ToScrollAlignment(align_y),
          ToScrollType(type),         make_visible_in_visual_viewport,
          ToScrollBehavior(behavior), is_for_scroll_sequence};
}

}  // namespace blink
