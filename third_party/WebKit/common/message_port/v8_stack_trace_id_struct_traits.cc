// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/message_port/v8_stack_trace_id_struct_traits.h"

namespace mojo {

bool StructTraits<blink::mojom::StackTraceId::DataView,
                  v8_inspector::V8StackTraceId>::
    Read(blink::mojom::StackTraceIdDataView data,
         v8_inspector::V8StackTraceId* out_stack_id) {
  *out_stack_id = v8_inspector::V8StackTraceId(
      reinterpret_cast<uintptr_t>(data.id()),
      std::make_pair(data.debugger_id_first(), data.debugger_id_second()));
  return true;
}

}  // namespace mojo
