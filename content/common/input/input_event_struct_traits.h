// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INPUT_INPUT_EVENT_STRUCT_TRAITS_H_
#define CONTENT_COMMON_INPUT_INPUT_EVENT_STRUCT_TRAITS_H_

#include "content/common/input/input_handler.mojom.h"

namespace content {
class InputEvent;
}

namespace mojo {

using InputEventUniquePtr = std::unique_ptr<content::InputEvent>;

template <>
struct StructTraits<content::mojom::EventDataView, InputEventUniquePtr> {
  static void* SetUpContext(const InputEventUniquePtr& handle);
  static void TearDownContext(const InputEventUniquePtr& handle, void* context);

  static blink::WebInputEvent::Type type(const InputEventUniquePtr& event);
  static int32_t modifiers(const InputEventUniquePtr& event);
  static double timestamp_seconds(const InputEventUniquePtr& event);
  static const ui::LatencyInfo& latency(const InputEventUniquePtr& event);
  static const content::mojom::KeyDataPtr& key_data(
      const InputEventUniquePtr& event,
      void* context);
  static const content::mojom::PointerDataPtr& pointer_data(
      const InputEventUniquePtr& event,
      void* context);
  static const content::mojom::GestureDataPtr& gesture_data(
      const InputEventUniquePtr& event,
      void* context);
  static const content::mojom::TouchDataPtr& touch_data(
      const InputEventUniquePtr& event,
      void* context);
  static bool Read(content::mojom::EventDataView r, InputEventUniquePtr* out);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_INPUT_INPUT_EVENT_STRUCT_TRAITS_H_
