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
  explicit VaapiJpegEncodeAccelerator(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);
  ~VaapiJpegEncodeAccelerator() override;

  // JpegEncodeAccelerator implementation.
  bool Initialize(JpegEncodeAccelerator::Client* client, Error* error) override;
  size_t GetMaxCodedBufferSize(const gfx::Size& picture_size) override;
  void Encode(scoped_refptr<media::VideoFrame> video_frame,
              int quality,
              BitstreamBuffer* bitstream_buffer) override;

 private:
  // An input video frame and the corresponding output buffer awaiting
  // consumption, provided by the client.
  struct EncodeRequest {
    EncodeRequest(scoped_refptr<media::VideoFrame> video_frame,
                  BitstreamBuffer* output_buffer,
                  int quality);
    ~EncodeRequest();

    scoped_refptr<media::VideoFrame> video_frame;
    BitstreamBuffer* output_buffer;
    int quality;
  };

  // The Encoder class is a collection of methods that run on
  // |encoder_task_runner_|.
  class Encoder {
    friend class VaapiJpegEncodeAccelerator;

   public:
    ~Encoder();

   private:
    Encoder(scoped_refptr<VaapiWrapper> vaapi_wrapper,
            scoped_refptr<base::SingleThreadTaskRunner> task_runner_,
            base::WeakPtr<VaapiJpegEncodeAccelerator> weak_this);

    void NotifyErrorFromEncoderThread(int video_frame_id, Error error);

    // Processes one encode |request|.
    void EncodeTask(std::unique_ptr<EncodeRequest> request);

    size_t cached_output_buffer_size_;
    VABufferID cached_output_buffer_id_;

    scoped_refptr<VaapiWrapper> vaapi_wrapper_;

    // ChildThread's task runner.
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

    // WeakPtr<> pointing to the parent VaapiJpegEncodeAccelerator's |this| for
    // use in posting tasks from the encoder thread back to the ChildThread.
    // Because the encoder thread is a member of VaapiJpegEncodeAccelerator, any
    // tasks running on the encoder thread is guaranteed that the parent
    // VaapiJpegEncodeAccelerator object is still alive.  As a result, tasks
    // posted from the encoder thread to the ChildThread should use
    // |weak_this_|.
    base::WeakPtr<VaapiJpegEncodeAccelerator> weak_this_;
  };

  // Notifies the client that an error has occurred and encoding cannot
  // continue.
  void NotifyError(int video_frame_id, Error error);

  void VideoFrameReady(int video_frame_id, size_t encoded_picture_size);

  // ChildThread's task runner.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // GPU IO task runner.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // The client of this class.
  Client* client_;

  // Use this to post tasks to encoder thread.
  scoped_refptr<base::SingleThreadTaskRunner> encoder_task_runner_;

  std::unique_ptr<Encoder> encoder_;

  // The WeakPtrFactory for |weak_this_|.
  base::WeakPtrFactory<VaapiJpegEncodeAccelerator> weak_this_factory_;

  DISALLOW_COPY_AND_ASSIGN(VaapiJpegEncodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_VAAPI_JPEG_ENCODE_ACCELERATOR_H_
