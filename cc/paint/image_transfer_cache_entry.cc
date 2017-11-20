// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/image_transfer_cache_entry.h"

#include "base/logging.h"
#include "base/numerics/checked_math.h"
#include "cc/paint/paint_op_reader.h"
#include "cc/paint/paint_op_writer.h"

namespace cc {

ClientImageTransferCacheEntry::ClientImageTransferCacheEntry(
    const SkImage* image,
    const SkColorSpace* target_color_space) {
  // This class must only be used with pre-decoded images.
  bool success = image->peekPixels(&pixmap_);
  DCHECK(success);

  // Compute and cache the size of the data.
  // We write the following:
  // - Image color type (uint32_t)
  // - Image width (uint32_t)
  // - Image height (uint32_t)
  // - Pixels size (uint32_t)
  // - Pixels (variable)
  base::CheckedNumeric<size_t> safe_size;
  safe_size += sizeof(uint32_t);  // color type
  safe_size += sizeof(uint32_t);  // width
  safe_size += sizeof(uint32_t);  // height
  safe_size += sizeof(uint32_t);  // pixels size
  safe_size += pixmap_.computeByteSize();
  safe_size += PaintOpWriter::HeaderBytes();
  size_ = safe_size.ValueOrDie();
  // TODO(ericrk): Handle colorspace.
}
ClientImageTransferCacheEntry::~ClientImageTransferCacheEntry() = default;

TransferCacheEntryType ClientImageTransferCacheEntry::Type() const {
  return TransferCacheEntryType::kImage;
}

size_t ClientImageTransferCacheEntry::SerializedSize() const {
  return size_;
}

bool ClientImageTransferCacheEntry::Serialize(size_t size,
                                              uint8_t* data) const {
  PaintOpWriter writer(data, size);
  writer.Write(static_cast<uint32_t>(pixmap_.colorType()));
  writer.Write(pixmap_.width());
  writer.Write(pixmap_.height());

  uint32_t pixmap_size =
      base::CheckedNumeric<uint32_t>(pixmap_.computeByteSize()).ValueOrDie();
  writer.Write(pixmap_size);
  writer.WriteData(pixmap_size, pixmap_.addr());
  // TODO(ericrk): Handle colorspace.

  if (writer.size() != size)
    return false;

  return true;
}

ServiceImageTransferCacheEntry::ServiceImageTransferCacheEntry() = default;
ServiceImageTransferCacheEntry::~ServiceImageTransferCacheEntry() = default;

TransferCacheEntryType ServiceImageTransferCacheEntry::Type() const {
  return TransferCacheEntryType::kImage;
}

size_t ServiceImageTransferCacheEntry::Size() const {
  return size_;
}

bool ServiceImageTransferCacheEntry::Deserialize(GrContext* context,
                                                 size_t size,
                                                 uint8_t* data) {
  PaintOpReader reader(data, size);
  uint32_t raw_color_type;
  reader.Read(&raw_color_type);
  SkColorType color_type = static_cast<SkColorType>(raw_color_type);
  uint32_t width;
  reader.Read(&width);
  uint32_t height;
  reader.Read(&height);
  SkImageInfo image_info =
      SkImageInfo::Make(width, height, color_type, kPremul_SkAlphaType);
  // TODO(ericrk): Handle colorspace.

  // Read pixels and create a pixmap.
  uint32_t pixel_size;
  reader.Read(&pixel_size);
  const volatile void* pixel_data = reader.ExtractReadableMemory(pixel_size);
  // Const-cast away the "volatile" on |pixel_data|. We specifically understand
  // that a malicious caller may change our pixels under us, and are OK with
  // this as the worst case scenario is visual corruption.
  SkPixmap pixmap(image_info, const_cast<const void*>(pixel_data),
                  image_info.minRowBytes());
  sk_sp<SkImage> image = SkImage::MakeFromRaster(pixmap, nullptr, nullptr);
  if (!image)
    return false;

  image_ = image->makeTextureImage(context, nullptr);

  return image_;
}

}  // namespace cc
