// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_
#define CHROME_GPU_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_

#include <vector>

#include "base/files/scoped_file.h"
#include "components/arc/video_accelerator/video_accelerator.h"
#include "media/base/video_codecs.h"
#include "media/base/video_types.h"
#include "media/video/picture.h"
#include "ui/gfx/geometry/size.h"

namespace chromeos {
namespace arc {

// Information of the shared memory bitstream buffer.
struct BitstreamBuffer {
  int32_t bitstream_id;
  base::ScopedFD ashmem_fd;
  uint32_t offset;
  uint32_t bytes_used;
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

// The IPC interface between Android and Chromium for video decoding. Input
// buffers are sent from Android side and get processed in Chromium and the
// output buffers are returned back to Android side.
class ArcVideoDecodeAcceleratorNew {
 public:
  enum Result {
    // Note: this enum is used for UMA reporting. The existing values should not
    // be rearranged, reused or removed and any additions should include updates
    // to UMA reporting and histograms.xml.
    SUCCESS = 0,
    ILLEGAL_STATE = 1,
    INVALID_ARGUMENT = 2,
    UNREADABLE_INPUT = 3,
    PLATFORM_FAILURE = 4,
    INSUFFICIENT_RESOURCES = 5,
    RESULT_MAX = 6,
  };

  struct Config {
    size_t num_input_buffers = 0;
    uint32_t input_pixel_format = 0;
    // TODO(owenlin): Add output_pixel_format. For now only the native pixel
    //                format of each VDA on Chromium is supported.
  };

  // The callbacks of the ArcVideoDecodeAcceleratorNew. The user of this class
  // should implement this interface.
  class Client {
   public:
    virtual ~Client() {}
    virtual void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                                       media::VideoPixelFormat format,
                                       uint32_t textures_per_buffer,
                                       const gfx::Size& dimensions,
                                       uint32_t texture_target) = 0;
    virtual void PictureReady(const media::Picture& picture) = 0;
    virtual void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer) = 0;
    virtual void NotifyFlushDone() = 0;
    virtual void NotifyResetDone() = 0;
    virtual void NotifyError(Result error) = 0;
  };

  virtual Result Initialize(media::VideoCodecProfile profile,
                            Client* client) = 0;
  virtual void Decode(BitstreamBuffer&& bitstream_buffer) = 0;
  virtual void AssignPictureBuffers(uint32_t count) = 0;
  virtual void ImportBufferForPicture(
      int32_t picture_id,
      base::ScopedFD ashmem_fd,
      std::vector<::arc::VideoFramePlane> planes) = 0;
  virtual void ReusePictureBuffer(int32_t picture_id) = 0;
  virtual void Reset() = 0;
  virtual void Flush() = 0;

  virtual ~ArcVideoDecodeAcceleratorNew() {}
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_
