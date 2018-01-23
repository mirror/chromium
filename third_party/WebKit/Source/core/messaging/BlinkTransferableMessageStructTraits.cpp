// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/messaging/BlinkTransferableMessageStructTraits.h"
#include "third_party/skia/include/core/SkBitmap.h"

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

  Vector<WTF::ArrayBufferContents> arrayBuffersContents;
  if (!data.ReadArrayBufferContentsArray(&arrayBuffersContents))
    return false;
  for (auto& contents : arrayBuffersContents) {
    WTF::ArrayBufferContents newContents;
    contents.Transfer(newContents);
    out->message->AddArrayBufferContents(newContents);
  }
  arrayBuffersContents.clear();

  // can possibly use
  // https://cs.chromium.org/chromium/src/third_party/skia/include/core/SkBitmap.h?gsn=PaintImage&q=file:skbitmap.h&l=959
  // to populate ArrayBufferContents
  Vector<SkBitmap> bitmapContents;
  if (!data.ReadImageBitmapContentsArray(&bitmapContents)) {
    return false;
  }
  Vector<scoped_refptr<blink::StaticBitmapImage>> unpackedBitmaps;
  for (const SkBitmap& contents : bitmapContents) {
    unsigned num_elements = contents.computeByteSize();
    unsigned element_byte_size = 1;
    WTF::ArrayBufferContents newContentsData(
        num_elements, element_byte_size,
        WTF::ArrayBufferContents::SharingType::kNotShared,
        WTF::ArrayBufferContents::InitializationPolicy::kDontInitialize);
    if (!newContentsData.Data()) {
      continue;
    }
    SkImageInfo newContentsInfo = std::move(contents.info());
    if (contents.readPixels(newContentsInfo, newContentsData.Data(),
                            newContentsInfo.minRowBytes(), 0, 0,
                            SkTransferFunctionBehavior::kIgnore)) {
      scoped_refptr<blink::StaticBitmapImage> newContents =
          blink::StaticBitmapImage::Create(newContentsData, newContentsInfo);
      out->message->AddImageBitmapContents(newContents);
    }
  }
  bitmapContents.clear();

  return true;
}

bool StructTraits<blink::mojom::blink::SerializedArrayBufferContents::DataView,
                  WTF::ArrayBufferContents>::
    Read(blink::mojom::blink::SerializedArrayBufferContents::DataView data,
         WTF::ArrayBufferContents* out) {
  mojo::ArrayDataView<uint8_t> user_contents;
  data.GetContentsDataView(&user_contents);
  const void* allocation_base =
      reinterpret_cast<const void*>(user_contents.data());
  WTF::ArrayBufferContents abc(user_contents.size(), 1,
                               WTF::ArrayBufferContents::kNotShared,
                               WTF::ArrayBufferContents::kDontInitialize);
  memcpy(abc.Data(), const_cast<void*>(allocation_base), user_contents.size());

  *out = std::move(abc);
  return true;
}

}  // namespace mojo
