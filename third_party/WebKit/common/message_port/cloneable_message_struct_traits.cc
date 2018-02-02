// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/message_port/cloneable_message_struct_traits.h"

#include "base/containers/span.h"

namespace mojo {

base::span<const uint8_t> StructTraits<
    blink::mojom::CloneableMessage::DataView,
    blink::CloneableMessage>::encoded_message(blink::CloneableMessage& input) {
  // Treat null encoded_message as empty. It really doesn't matter what address
  // is used for the empty span, but it can't simply be
  // owned_encoded_message.data() since that would be null too with an empty
  // vector
  if (!input.encoded_message.data())
    return base::make_span(reinterpret_cast<const uint8_t*>(&input), 0);
  return input.encoded_message;
}

bool StructTraits<blink::mojom::CloneableMessage::DataView,
                  blink::CloneableMessage>::
    Read(blink::mojom::CloneableMessage::DataView data,
         blink::CloneableMessage* out) {
  if (!data.ReadEncodedMessage(&out->owned_encoded_message) ||
      !data.ReadBlobs(&out->blobs)) {
    return false;
  }

  out->encoded_message = out->owned_encoded_message;
  out->stack_trace_id = data.stack_trace_id();
  out->stack_trace_debugger_id_first = data.stack_trace_debugger_id_first();
  out->stack_trace_debugger_id_second = data.stack_trace_debugger_id_second();
  return true;
}

}  // namespace mojo
