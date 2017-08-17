// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
#define CHROME_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_

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
class ArcVideoDecodeAccelerator {
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

  // The callbacks of the ArcVideoDecodeAccelerator similar
  // to media::VideoDecodeAccelerator::Client.
  // The user of this class should implement this interface.
  class Client {
   public:
    virtual ~Client() {}

    // Callback to tell client how many and what size of buffers to provide.
    // by the VDA, or PIXEL_FORMAT_UNKNOWN if the underlying platform handles
    // |format| indicates what format the decoded frames will be produced in
    // by the VDA, or PIXEL_FORMAT_UNKNOWN if the underlying platform handles
    // this transparently.
    virtual void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                                       media::VideoPixelFormat format,
                                       uint32_t textures_per_buffer,
                                       const gfx::Size& dimensions,
                                       uint32_t texture_target) = 0;

    // Called when decoded pictures is ready to be displayed.
    virtual void PictureReady(const media::Picture& picture) = 0;

    // Called as a notification that decoded has decoded the end of the current
    // bitstream buffer.
    virtual void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer) = 0;

    // Called as a completion notification for Flush().
    virtual void NotifyFlushDone() = 0;

    // Called as a completion notification for Reset().
    virtual void NotifyResetDone() = 0;

    // Called when an asynchronous error happens. The errors in Initialize()
    // will not be reported here, but will be indicated by a return value there.
    virtual void NotifyError(Result error) = 0;
  };

  // Initializes the ArcVideoDecodeAccelerator with video codec profile.
  // This must be called before any other methods.
  // This call is synchronous and returns SUCCESS
  // iff initialization is successful.
  virtual Result Initialize(media::VideoCodecProfile profile,
                            Client* client) = 0;

  // Decodes given bitstream buffer that contains at most one frame.
  // BitstreamBuffer needs to be passed as right value, because
  // |bitstream_buffer.ashmem_fd| should be released when its file descriptor
  // is passed to VDA.
  virtual void Decode(chromeos::arc::BitstreamBuffer&& bitstream_buffer) = 0;

  // Sets the number of output buffers.
  virtual void AssignPictureBuffers(uint32_t count) = 0;

  // Import a handle as backing memory for picture buffer associated with
  // |picture_id|. The handle is created with |ashmem_fd| and |planes|.
  virtual void ImportBufferForPicture(
      int32_t picture_id,
      base::ScopedFD ashmem_fd,
      std::vector<::arc::VideoFramePlane> planes) = 0;

  // Reuse the picture buffer associated with |picture_id|.
  virtual void ReusePictureBuffer(int32_t picture_id) = 0;

  // Resets the accelerator. When it is done, Client::OnResetDone() will
  // be called. Afterwards, all buffers won't be accessed by the accelerator
  // and there won't be more callbacks.
  virtual void Reset() = 0;

  // Flushes the accelerator. After all the output buffers pending decode have
  // been returned to client by OnBufferDone(), Client::OnFlushDone() will be
  // called.
  virtual void Flush() = 0;

  virtual ~ArcVideoDecodeAccelerator() {}
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
