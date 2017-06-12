// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_IPC_SERVICE_GPU_JPEG_DECODE_ACCELERATOR_H_
#define MEDIA_GPU_IPC_SERVICE_GPU_JPEG_DECODE_ACCELERATOR_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "media/gpu/mojo/jpeg_decoder.mojom.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
struct BindSourceInfo;
}

namespace media {

class GpuJpegDecodeAcceleratorFactoryProvider {
 public:
  using CreateAcceleratorCB =
      base::Callback<std::unique_ptr<JpegDecodeAccelerator>(
          scoped_refptr<base::SingleThreadTaskRunner>)>;

  // Static query for JPEG supported. This query calls the appropriate
  // platform-specific version.
  static bool IsAcceleratedJpegDecodeSupported();

  static std::vector<CreateAcceleratorCB> GetAcceleratorFactories();
};

// TODO(c.padhi): Move GpuJpegDecodeAccelerator to media/gpu/mojo, see
// http://crbug.com/699255.
class GpuJpegDecodeAccelerator : public mojom::GpuJpegDecodeAccelerator,
                                 public JpegDecodeAccelerator::Client {
 public:
  static void Create(scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
                     const service_manager::BindSourceInfo& source_info,
                     mojom::GpuJpegDecodeAcceleratorRequest request);

  ~GpuJpegDecodeAccelerator() override;

  // JpegDecodeAccelerator::Client implementation.
  void VideoFrameReady(int32_t buffer_id) override;
  void NotifyError(int32_t buffer_id,
                   JpegDecodeAccelerator::Error error) override;

 private:
  // This constructor internally calls
  // GpuJpegDecodeAcceleratorFactoryProvider::GetAcceleratorFactories() to
  // fill |accelerator_factory_functions_|.
  explicit GpuJpegDecodeAccelerator(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  // mojom::GpuJpegDecodeAccelerator implementation.
  void Initialize(InitializeCallback callback) override;
  void Decode(mojom::BitstreamBufferPtr input_buffer,
              const gfx::Size& coded_size,
              mojo::ScopedSharedBufferHandle output_handle,
              uint32_t output_buffer_size,
              DecodeCallback callback) override;
  void Uninitialize() override;

  void DecodeOnIOThread(mojom::BitstreamBufferPtr input_buffer,
                        const gfx::Size& coded_size,
                        mojo::ScopedSharedBufferHandle output_handle,
                        uint32_t output_buffer_size);
  void NotifyDecodeStatus(DecodeCallback callback,
                          int32_t bitstream_buffer_id,
                          mojom::Error error);

  const std::vector<
      GpuJpegDecodeAcceleratorFactoryProvider::CreateAcceleratorCB>
      accelerator_factory_functions_;

  // GPU IO task runner.
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  base::Callback<void(int32_t, mojom::Error)> notify_decode_status_cb_;

  std::unique_ptr<JpegDecodeAccelerator> accelerator_;

  THREAD_CHECKER(thread_checker_);

  // Used to get weak pointer to self for use on IO thread.
  base::WeakPtrFactory<GpuJpegDecodeAccelerator> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(GpuJpegDecodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_GPU_IPC_SERVICE_GPU_JPEG_DECODE_ACCELERATOR_H_
