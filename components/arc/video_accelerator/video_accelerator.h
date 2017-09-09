// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_ACCELERATOR_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_ACCELERATOR_H_

namespace arc {

struct VideoFramePlane {
  int32_t offset;  // in bytes
  int32_t stride;  // in bytes
};

// Format specification of the picture buffer request.
struct PictureBufferFormat {
  uint32_t pixel_format;
  uint32_t buffer_size;

  // minimal number of buffers required to process the video.
  uint32_t min_num_buffers;
  uint32_t coded_width;
  uint32_t coded_height;
};

}  // namespace arc

#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_VIDEO_ACCELERATOR_H_
