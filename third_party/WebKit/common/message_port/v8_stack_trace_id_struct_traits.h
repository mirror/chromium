// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_V8_STACK_TRACE_ID_STRUCT_TRAITS_H_
#define THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_V8_STACK_TRACE_ID_STRUCT_TRAITS_H_

#include "third_party/WebKit/common/message_port/message_port.mojom.h"

namespace mojo {

template <>
struct BLINK_COMMON_EXPORT StructTraits<blink::mojom::StackTraceId::DataView,
                                        v8_inspector::V8StackTraceId> {
  static uint64_t id(const v8_inspector::V8StackTraceId& stack_id) {
    return reinterpret_cast<uint64_t>(stack_id.id);
  }

  static int64_t debugger_id_first(
      const v8_inspector::V8StackTraceId& stack_id) {
    return stack_id.debugger_id.first;
  }

  static int64_t debugger_id_second(
      const v8_inspector::V8StackTraceId& stack_id) {
    return stack_id.debugger_id.second;
  }

  static bool Read(blink::mojom::StackTraceIdDataView,
                   v8_inspector::V8StackTraceId*);
};

}  // namespace mojo

#endif  // THIRD_PARTY_WEBKIT_COMMON_MESSAGE_PORT_V8_STACK_TRACE_ID_STRUCT_TRAITS_H_
