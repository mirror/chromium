// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebRemoteScrollPropertiesStructTraits_h
#define WebRemoteScrollPropertiesStructTraits_h

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/WebKit/public/platform/WebRemoteScrollProperties.h"
#include "third_party/WebKit/public/platform/remote_scroll_properties.mojom-shared.h"

namespace mojo {

template <>
struct EnumTraits<blink::mojom::RemoteScrollAlignment,
                  ::blink::WebRemoteScrollProperties::Alignment> {
  static blink::mojom::RemoteScrollAlignment ToMojom(
      ::blink::WebRemoteScrollProperties::Alignment alignment) {
    switch (alignment) {
      case ::blink::WebRemoteScrollProperties::kCenterIfNeeded:
        return ::blink::mojom::RemoteScrollAlignment::kCenterIfNeeded;
      case ::blink::WebRemoteScrollProperties::kToEdgeIfNeeded:
        return ::blink::mojom::RemoteScrollAlignment::kToEdgeIfNeeded;
      case ::blink::WebRemoteScrollProperties::kCenterAlways:
        return ::blink::mojom::RemoteScrollAlignment::kCenterAlways;
      case ::blink::WebRemoteScrollProperties::kTopAlways:
        return ::blink::mojom::RemoteScrollAlignment::kTopAlways;
      case ::blink::WebRemoteScrollProperties::kBottomAlways:
        return ::blink::mojom::RemoteScrollAlignment::kBottomAlways;
      case ::blink::WebRemoteScrollProperties::kLeftAlways:
        return ::blink::mojom::RemoteScrollAlignment::kLeftAlways;
      case ::blink::WebRemoteScrollProperties::kRightAlways:
        return ::blink::mojom::RemoteScrollAlignment::kRightAlways;
    }
  }

  static bool FromMojom(::blink::mojom::RemoteScrollAlignment alignment,
                        ::blink::WebRemoteScrollProperties::Alignment* out) {
    switch (alignment) {
      case ::blink::mojom::RemoteScrollAlignment::kCenterIfNeeded:
        *out = ::blink::WebRemoteScrollProperties::kCenterIfNeeded;
        return true;
      case ::blink::mojom::RemoteScrollAlignment::kToEdgeIfNeeded:
        *out = ::blink::WebRemoteScrollProperties::kToEdgeIfNeeded;
        return true;
      case ::blink::mojom::RemoteScrollAlignment::kCenterAlways:
        *out = ::blink::WebRemoteScrollProperties::kCenterAlways;
        return true;
      case ::blink::mojom::RemoteScrollAlignment::kTopAlways:
        *out = ::blink::WebRemoteScrollProperties::kTopAlways;
        return true;
      case ::blink::mojom::RemoteScrollAlignment::kBottomAlways:
        *out = ::blink::WebRemoteScrollProperties::kBottomAlways;
        return true;
      case ::blink::mojom::RemoteScrollAlignment::kLeftAlways:
        *out = ::blink::WebRemoteScrollProperties::kLeftAlways;
        return true;
      case ::blink::mojom::RemoteScrollAlignment::kRightAlways:
        *out = ::blink::WebRemoteScrollProperties::kRightAlways;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<blink::mojom::RemoteScrollType,
                  ::blink::WebRemoteScrollProperties::Type> {
  static blink::mojom::RemoteScrollType ToMojom(
      ::blink::WebRemoteScrollProperties::Type type) {
    switch (type) {
      case ::blink::WebRemoteScrollProperties::kUser:
        return ::blink::mojom::RemoteScrollType::kUser;
      case ::blink::WebRemoteScrollProperties::kProgrammatic:
        return ::blink::mojom::RemoteScrollType::kProgrammatic;
      case ::blink::WebRemoteScrollProperties::kClamping:
        return ::blink::mojom::RemoteScrollType::kClamping;
      case ::blink::WebRemoteScrollProperties::kCompositor:
        return ::blink::mojom::RemoteScrollType::kCompositor;
      case ::blink::WebRemoteScrollProperties::kAnchoring:
        return ::blink::mojom::RemoteScrollType::kAnchoring;
      case ::blink::WebRemoteScrollProperties::kSequenced:
        return ::blink::mojom::RemoteScrollType::kSequenced;
    }
  }

  static bool FromMojom(::blink::mojom::RemoteScrollType type,
                        ::blink::WebRemoteScrollProperties::Type* out) {
    switch (type) {
      case ::blink::mojom::RemoteScrollType::kUser:
        *out = ::blink::WebRemoteScrollProperties::kUser;
        return true;
      case ::blink::mojom::RemoteScrollType::kProgrammatic:
        *out = ::blink::WebRemoteScrollProperties::kProgrammatic;
        return true;
      case ::blink::mojom::RemoteScrollType::kClamping:
        *out = ::blink::WebRemoteScrollProperties::kClamping;
        return true;
      case ::blink::mojom::RemoteScrollType::kCompositor:
        *out = ::blink::WebRemoteScrollProperties::kCompositor;
        return true;
      case ::blink::mojom::RemoteScrollType::kAnchoring:
        *out = ::blink::WebRemoteScrollProperties::kAnchoring;
        return true;
      case ::blink::mojom::RemoteScrollType::kSequenced:
        *out = ::blink::WebRemoteScrollProperties::kSequenced;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<blink::mojom::RemoteScrollBehavior,
                  ::blink::WebRemoteScrollProperties::Behavior> {
  static blink::mojom::RemoteScrollBehavior ToMojom(
      ::blink::WebRemoteScrollProperties::Behavior behavior) {
    switch (behavior) {
      case ::blink::WebRemoteScrollProperties::kAuto:
        return ::blink::mojom::RemoteScrollBehavior::kAuto;
      case ::blink::WebRemoteScrollProperties::kInstant:
        return ::blink::mojom::RemoteScrollBehavior::kInstant;
      case ::blink::WebRemoteScrollProperties::kSmooth:
        return ::blink::mojom::RemoteScrollBehavior::kSmooth;
    }
  }

  static bool FromMojom(::blink::mojom::RemoteScrollBehavior behavior,
                        ::blink::WebRemoteScrollProperties::Behavior* out) {
    switch (behavior) {
      case ::blink::mojom::RemoteScrollBehavior::kAuto:
        *out = ::blink::WebRemoteScrollProperties::kAuto;
        return true;
      case ::blink::mojom::RemoteScrollBehavior::kInstant:
        *out = ::blink::WebRemoteScrollProperties::kInstant;
        return true;
      case ::blink::mojom::RemoteScrollBehavior::kSmooth:
        *out = ::blink::WebRemoteScrollProperties::kSmooth;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<blink::mojom::RemoteScrollPropertiesDataView,
                    ::blink::WebRemoteScrollProperties> {
  static ::blink::WebRemoteScrollProperties::Alignment align_x(
      const ::blink::WebRemoteScrollProperties& r) {
    return r.align_x;
  }

  static ::blink::WebRemoteScrollProperties::Alignment align_y(
      const ::blink::WebRemoteScrollProperties& r) {
    return r.align_y;
  }

  static ::blink::WebRemoteScrollProperties::Type type(
      const ::blink::WebRemoteScrollProperties& r) {
    return r.type;
  }

  static bool make_visible_in_visual_viewport(
      const ::blink::WebRemoteScrollProperties& r) {
    return r.make_visible_in_visual_viewport;
  }

  static ::blink::WebRemoteScrollProperties::Behavior behavior(
      const ::blink::WebRemoteScrollProperties& r) {
    return r.behavior;
  }

  static bool is_for_scroll_sequence(
      const ::blink::WebRemoteScrollProperties& r) {
    return r.is_for_scroll_sequence;
  }

  static bool Read(blink::mojom::RemoteScrollPropertiesDataView data,
                   ::blink::WebRemoteScrollProperties* out) {
    if (!data.ReadAlignX(&out->align_x) || !data.ReadAlignY(&out->align_y) ||
        !data.ReadType(&out->type))
      return false;
    out->make_visible_in_visual_viewport =
        data.make_visible_in_visual_viewport();
    if (!data.ReadBehavior(&out->behavior))
      return false;
    out->is_for_scroll_sequence = data.is_for_scroll_sequence();
    return true;
  }
};

}  // namespace mojo

#endif  // WebRemoteScrollPropertiesStructTraits_h
