// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_JPEG_ENCODER_H_
#define MEDIA_GPU_VAAPI_JPEG_ENCODER_H_

#include "base/macros.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/vaapi_wrapper.h"

namespace media {

struct JpegParseResult;

// A JPEG Encoder that utilizes VA-API hardware video encode acceleration on
// Intel systems. Provides functionality to allow plugging VAAPI HW
// acceleration into the JpegEncodeAccelerator framework.
//
// Clients of this class are expected to manage VA surfaces created via
// VaapiWrapper, parse JPEG picture via ParseJpegPicture, and then pass
// them to this class.
class MEDIA_GPU_EXPORT VaapiJpegEncoder {
 public:
  // Encode a JPEG picture. It will fill VA-API parameters and call
  // corresponding VA-API methods according to parsed JPEG result
  // |parse_result|. Encoded data will be outputted to the given |va_surface|.
  // Return false on failure.
  // |vaapi_wrapper| should be initialized in kEncode mode with
  // VAProfileJPEGBaseline profile.
  // |va_surface| should be created with size at least as large as the picture
  // size.
  static bool Encode(VaapiWrapper* vaapi_wrapper,
                     int width, int height,
                     VASurfaceID va_surface);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(VaapiJpegEncoder);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_JPEG_ENCODER_H_
