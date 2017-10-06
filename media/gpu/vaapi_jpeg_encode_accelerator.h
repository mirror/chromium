// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_VAAPI_JPEG_ENCODE_ACCELERATOR_H_
#define MEDIA_GPU_VAAPI_JPEG_ENCODE_ACCELERATOR_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/base/bitstream_buffer.h"
#include "media/gpu/media_gpu_export.h"
#include "media/gpu/shared_memory_region.h"
#include "media/gpu/vaapi_wrapper.h"
#include "media/video/jpeg_encode_accelerator.h"

namespace media {

// Class to provide JPEG encode acceleration for Intel systems with hardware
// support for it, and on which libva is available.
// Encoding tasks are performed in a separate encoding thread.
//
// Threading/life-cycle: this object is created & destroyed on the GPU
// ChildThread.  A few methods on it are called on the encoder thread which is
// stopped during |this->Destroy()|, so any tasks posted to the encoder thread
// can assume |*this| is still alive.  See |weak_this_| below for more details.
class MEDIA_GPU_EXPORT VaapiJpegEncodeAccelerator
    : public JpegEncodeAccelerator {
 public:
  VaapiJpegEncodeAccelerator(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);
  ~VaapiJpegEncodeAccelerator() override;

  // JpegEncodeAccelerator implementation.
  bool Initialize(JpegEncodeAccelerator::Client* client) override;
  size_t GetMaxCodedBufferSize(int width, int height) override;
  void Encode(const scoped_refptr<media::VideoFrame>& video_frame,
              int quality,
              BitstreamBuffer* bitstream_buffer) override;
  bool IsSupported() override;

 private:
  // An input video frame and the corresponding output buffer awaiting
  // consumption, provided by the client.
  struct EncodeRequest {
    EncodeRequest(const scoped_refptr<media::VideoFrame>& video_frame,
                  std::unique_ptr<SharedMemoryRegion> shm,
                  int quality);
    ~EncodeRequest();

    scoped_refptr<media::VideoFrame> video_frame;
    std::unique_ptr<SharedMemoryRegion> shm;
    int quality;
  };

  // Notifies the client that an error has occurred and encoding cannot
  // continue.
  void NotifyError(int video_frame_id, Error error);
  void NotifyErrorFromEncoderThread(int video_frame_id, Error error);
  void VideoFrameReady(int video_frame_id, size_t encoded_picture_size);

  // Processes one encode |request|.
  void EncodeTask(const std::unique_ptr<EncodeRequest>& request);

  // ChildThread's task runner.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // GPU IO task runner.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // The client of this class.
  Client* client_;

  // WeakPtr<> pointing to |this| for use in posting tasks from the encoder
  // thread back to the ChildThread.  Because the encoder thread is a member of
  // this class, any task running on the encoder thread is guaranteed that this
  // object is still alive.  As a result, tasks posted from ChildThread to
  // encoder thread should use base::Unretained(this), and tasks posted from the
  // encoder thread to the ChildThread should use |weak_this_|.
  base::WeakPtr<VaapiJpegEncodeAccelerator> weak_this_;

  scoped_refptr<VaapiWrapper> vaapi_wrapper_;

  base::Thread encoder_thread_;

  // Use this to post tasks to |encoder_thread_| instead of
  // |encoder_thread_.task_runner()| because the latter will be NULL once
  // |encoder_thread_.Stop()| returns.
  scoped_refptr<base::SingleThreadTaskRunner> encoder_task_runner_;

  base::TimeTicks start_encode_timestamp_;

  // The WeakPtrFactory for |weak_this_|.
  base::WeakPtrFactory<VaapiJpegEncodeAccelerator> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(VaapiJpegEncodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_JPEG_ENCODE_ACCELERATOR_H_
