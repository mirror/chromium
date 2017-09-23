// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_

#include "components/arc/common/video_decode_accelerator.mojom.h"
// #include "media/base/bitstream_buffer.h"

namespace mojo {
/* template <>
 * struct StructTraits<arc::mojom::BitstreamBufferDataView,
 *                     media::BitstreamBuffer> {
 *   static int32_t bitstream_id(const media::BitstreamBuffer& b) {
 *     return b.id();
 *   }
 *   static uint32_t offset(const media::BitstreamBuffer& b) {
 *     return b.offset();
 *   }
 *   static uint32_t bytes_used(const media::BitstreamBuffer& b) {
 *     return b.size();
 *   }
 *   static mojo::ScopedSharedBufferHandle memory_handle(
 *       const media::BitstreamBuffer& b);
 *
 *   static bool Read(arc::mojom::BitstreamBufferDataView data,
 *                    media::BitstreamBuffer* out);
 * }; */

}  // namespace mojo

#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_DECODE_ACCELERATOR_STRUCT_TRAITS_H_
