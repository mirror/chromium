// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/video_accelerator/video_decode_accelerator_struct_traits.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace mojo {
// static
// mojo::ScopedSharedBufferHandle StructTraits<
//     arc::mojom::BitstreamBufferDataView,
//     media::BitstreamBuffer>::memory_handle(const media::BitstreamBuffer& b) {
//   return mojo::WrapSharedMemoryHandle(b.handle(), b.handle().GetSize(),
//                                       false);
// }
// static
// bool StructTraits<arc::mojom::BitstreamBufferDataView,
// media::BitstreamBuffer>::
//     Read(arc::mojom::BitstreamBufferDataView data,
//          media::BitstreamBuffer* out) {
//   mojo::ScopedSharedBufferHandle handle = data.TakeMemoryHandle();
//   LOG(ERROR) << "BitstreamBuffer Convertion";
//   if (!handle.is_valid()) {
//     LOG(ERROR) << "handle is invalid.";
//     return false;
//   }

//   base::SharedMemoryHandle memory_handle;
//   MojoResult unwrap_result = mojo::UnwrapSharedMemoryHandle(
//       std::move(handle), &memory_handle, nullptr, nullptr);
//   if (unwrap_result != MOJO_RESULT_OK)
//     return false;

//   *out = media::BitstreamBuffer(data.bitstream_id(),
//                                 std::move(memory_handle),
//                                 data.bytes_used(), data.offset());
//    return true;
// }

}  // namespace mojo
