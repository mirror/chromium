// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/message_port/transferable_message_struct_traits.h"

#include "base/containers/span.h"
#include "third_party/WebKit/common/message_port/cloneable_message_struct_traits.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace mojo {

bool StructTraits<blink::mojom::TransferableMessage::DataView,
                  blink::TransferableMessage>::
    Read(blink::mojom::TransferableMessage::DataView data,
         blink::TransferableMessage* out) {
  std::vector<mojo::ScopedMessagePipeHandle> ports;
  std::vector<blink::mojom::SerializedArrayBufferContentsPtr> arrayBufferArray;
  std::vector<SkBitmap> imageBitmaps;
  if (!data.ReadMessage(static_cast<blink::CloneableMessage*>(out)) ||
      !data.ReadArrayBufferContentsArray(&arrayBufferArray) ||
      !data.ReadImageBitmapContentsArray(&imageBitmaps) ||
      !data.ReadPorts(&ports)) {
    return false;
  }

  out->ports = blink::MessagePortChannel::CreateFromHandles(std::move(ports));
  out->arrayBufferContentsArray = std::move(arrayBufferArray);
  out->imageBitmapContentsArray = std::move(imageBitmaps);
  return true;
}

}  // namespace mojo
