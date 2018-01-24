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
  Vector<WTF::ArrayBufferContents> array_buffer_contents_array;
  Vector<SkBitmap> bitmap_contents_array;
  if (!data.ReadMessage(static_cast<blink::BlinkCloneableMessage*>(out)) ||
      !data.ReadArrayBufferContentsArray(&array_buffer_contents_array) ||
      !data.ReadImageBitmapContentsArray(&bitmap_contents_array) ||
      !data.ReadPorts(&ports)) {
    return false;
  }

  out->ports.ReserveInitialCapacity(ports.size());
  out->ports.AppendRange(std::make_move_iterator(ports.begin()),
                         std::make_move_iterator(ports.end()));

  for (auto& contents : array_buffer_contents_array) {
    WTF::ArrayBufferContents newContents;
    contents.Transfer(newContents);
    out->message->AddArrayBufferContents(newContents);
  }

  for (const SkBitmap& contents : bitmap_contents_array) {
    auto handle = WTF::ArrayBufferContents::CreateDataHandle(
        contents.computeByteSize(), WTF::ArrayBufferContents::kZeroInitialize);
    if (!handle)
      return false;
    WTF::ArrayBufferContents array_buffer_contents(
        std::move(handle), WTF::ArrayBufferContents::kNotShared);
    if (!array_buffer_contents.Data()) {
      return false;
    }
    SkImageInfo info = contents.info();
    if (!contents.readPixels(info, array_buffer_contents.Data(),
                             info.minRowBytes(), 0, 0,
                             SkTransferFunctionBehavior::kIgnore)) {
      return false;
    }
    scoped_refptr<blink::StaticBitmapImage> bitmap =
        blink::StaticBitmapImage::Create(array_buffer_contents,
                                         std::move(info));
    out->message->AddImageBitmapContents(bitmap);
  }

  return true;
}

bool StructTraits<blink::mojom::blink::SerializedArrayBufferContents::DataView,
                  WTF::ArrayBufferContents>::
    Read(blink::mojom::blink::SerializedArrayBufferContents::DataView data,
         WTF::ArrayBufferContents* out) {
  base::span<uint8_t> mojo_contents;
  if (!data.ReadContents(&mojo_contents))
    return false;
  //  data.GetContentsDataView(&mojo_contents);
  auto handle = WTF::ArrayBufferContents::CreateDataHandle(
      mojo_contents.size(), WTF::ArrayBufferContents::kZeroInitialize);
  if (!handle)
    return false;
  WTF::ArrayBufferContents array_buffer_contents(
      std::move(handle), WTF::ArrayBufferContents::kNotShared);
  void* allocation_base = reinterpret_cast<void*>(mojo_contents.begin());
  memcpy(array_buffer_contents.Data(), allocation_base, mojo_contents.size());

  *out = std::move(array_buffer_contents);
  return true;
}

Vector<SkBitmap>
StructTraits<blink::mojom::blink::TransferableMessage::DataView,
             blink::BlinkTransferableMessage>::
    imageBitmapContentsArray(blink::BlinkCloneableMessage& input) {
  Vector<SkBitmap> out;
  for (scoped_refptr<blink::StaticBitmapImage>& bitmap_contents :
       input.message->GetImageBitmapContentsArray()) {
    SkBitmap bitmap;
    const sk_sp<SkImage> image =
        bitmap_contents->PaintImageForCurrentFrame().GetSkImage();
    if (image &&
        image->asLegacyBitmap(
            &bitmap, SkImage::LegacyBitmapMode::kRO_LegacyBitmapMode)) {
      out.push_back(std::move(bitmap));
    }
  }
  return out;
}

}  // namespace mojo
