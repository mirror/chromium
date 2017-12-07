// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/BlinkTransferableMessageStructTraits.h"

namespace mojo {

bool StructTraits<blink::mojom::blink::TransferableMessage::DataView,
                  blink::BlinkTransferableMessage>::
    Read(blink::mojom::blink::TransferableMessage::DataView data,
         blink::BlinkTransferableMessage* out) {
  Vector<mojo::ScopedMessagePipeHandle> ports;
  if (!data.ReadMessage(static_cast<blink::BlinkCloneableMessage*>(out)) ||
      !data.ReadPorts(&ports)) {
    return false;
  }

  out->ports.ReserveInitialCapacity(ports.size());
  out->ports.AppendRange(std::make_move_iterator(ports.begin()),
                         std::make_move_iterator(ports.end()));

  blink::mojom::StackTraceIdDataView stack_trace_id_data_view;
  data.GetSenderStackTraceIdDataView(&stack_trace_id_data_view);
  if (stack_trace_id_data_view.is_null())
    return false;
  out->sender_stack_trace_id = v8_inspector::V8StackTraceId(
      reinterpret_cast<uintptr_t>(stack_trace_id_data_view.id()),
      std::make_pair(stack_trace_id_data_view.debugger_id_first(),
                     stack_trace_id_data_view.debugger_id_second()));
  return true;
}

}  // namespace mojo
