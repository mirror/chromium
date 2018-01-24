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
  Vector<WTF::ArrayBufferContents> arrayBuffersContents;
  Vector<SkBitmap> bitmapContents;
  if (!data.ReadMessage(static_cast<blink::BlinkCloneableMessage*>(out)) ||
      !data.ReadArrayBufferContentsArray(&arrayBuffersContents) ||
      !data.ReadImageBitmapContentsArray(&bitmapContents) ||
      !data.ReadPorts(&ports)) {
    return false;
  }

  out->ports.ReserveInitialCapacity(ports.size());
  out->ports.AppendRange(std::make_move_iterator(ports.begin()),
                         std::make_move_iterator(ports.end()));

  for (auto& contents : arrayBuffersContents) {
    WTF::ArrayBufferContents newContents;
    contents.Transfer(newContents);
    out->message->AddArrayBufferContents(newContents);
  }

  Vector<scoped_refptr<blink::StaticBitmapImage>> unpackedBitmaps;
  for (const SkBitmap& contents : bitmapContents) {
    unsigned num_elements = static_cast<unsigned>(contents.computeByteSize());
    unsigned element_byte_size = 1;
    WTF::ArrayBufferContents newContentsData(
        num_elements, element_byte_size,
        WTF::ArrayBufferContents::SharingType::kNotShared,
        WTF::ArrayBufferContents::InitializationPolicy::kDontInitialize);
    if (!newContentsData.Data()) {
      return false;
    }
    SkImageInfo info = contents.info();
    if (!contents.readPixels(info, newContentsData.Data(), info.minRowBytes(),
                             0, 0, SkTransferFunctionBehavior::kIgnore)) {
      return false;
    }
    scoped_refptr<blink::StaticBitmapImage> newContents =
        blink::StaticBitmapImage::Create(newContentsData, std::move(info));
    out->message->AddImageBitmapContents(newContents);
  }

  return true;
}

bool StructTraits<blink::mojom::blink::SerializedArrayBufferContents::DataView,
                  WTF::ArrayBufferContents>::
    Read(blink::mojom::blink::SerializedArrayBufferContents::DataView data,
         WTF::ArrayBufferContents* out) {
  mojo::ArrayDataView<uint8_t> mojo_contents;
  data.GetContentsDataView(&mojo_contents);
  WTF::ArrayBufferContents array_buffer_contents(
      static_cast<unsigned>(mojo_contents.size()), 1,
      WTF::ArrayBufferContents::kNotShared,
      WTF::ArrayBufferContents::kDontInitialize);
  void* allocation_base =
      const_cast<void*>(reinterpret_cast<const void*>(mojo_contents.data()));
  memcpy(array_buffer_contents.Data(), allocation_base, mojo_contents.size());

  *out = std::move(array_buffer_contents);
  return true;
}

Vector<SkBitmap>
StructTraits<blink::mojom::blink::TransferableMessage::DataView,
             blink::BlinkTransferableMessage>::
    imageBitmapContentsArray(blink::BlinkCloneableMessage& input) {
  Vector<SkBitmap> out;
  for (scoped_refptr<blink::StaticBitmapImage>& bitmapContents :
       input.message->GetImageBitmapContentsArray()) {
    SkBitmap bitmap;
    bitmapContents->PaintImageForCurrentFrame();
    const sk_sp<SkImage> image =
        bitmapContents->PaintImageForCurrentFrame().GetSkImage();
    if (image &&
        image->asLegacyBitmap(
            &bitmap, SkImage::LegacyBitmapMode::kRO_LegacyBitmapMode)) {
      out.push_back(std::move(bitmap));
    }
  }
  return out;
}

}  // namespace mojo
