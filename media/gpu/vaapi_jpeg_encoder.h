// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_JPEG_ENCODER_H_
#define MEDIA_GPU_VAAPI_JPEG_ENCODER_H_

#include "base/macros.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/vaapi_wrapper.h"

namespace media {

// A collection of functions that utilize VA-API hardware video encode
// acceleration on Intel systems. Provides functionality to allow plugging VAAPI
// HW acceleration into the JpegEncodeAccelerator framework.
//
// Clients of the functions are expected to manage VA surfaces and VA buffers
// created via VaapiWrapper, and pass them to this class.
namespace vaapi_jpeg_encoder {

// Encode a JPEG picture. It will fill VA-API parameters and call
// corresponding VA-API methods according to |input_size|.
// |vaapi_wrapper| should be initialized in VaapiWrapper::CodecMode::kEncodeJpeg
// mode with VAProfileJPEGBaseline profile.
// |quality| is the JPEG image quality
// |surface_id| is the VA surface that contains input image.
// |output_buffer_id| is the ID of VA buffer that encoded image will be
// stored. The size of it should be at least as large as
// GetMaxCodedBufferSize().
// Return false on failure.
bool Encode(VaapiWrapper* vaapi_wrapper,
            const gfx::Size& input_size,
            int quality,
            VASurfaceID surface_id,
            VABufferID output_buffer_id);

// Gets the maximum possible encoded result size.
size_t GetMaxCodedBufferSize(int width, int height);

}  // namespace vaapi_jpeg_encoder
}  // namespace media

#endif  // MEDIA_GPU_VAAPI_JPEG_ENCODER_H_
