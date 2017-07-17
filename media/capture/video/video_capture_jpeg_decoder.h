// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_JPEG_DECODER_H_
#define MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_JPEG_DECODER_H_

#include "base/callback.h"
#include "media/capture/capture_export.h"
#include "media/capture/mojo/video_capture_types.mojom.h"
#include "media/capture/video/video_capture_device.h"
#include "media/capture/video/video_frame_receiver.h"

namespace ui {
  class Gpu;
}

namespace gpu {
  class GpuChannelHost;
}

namespace service_manager {
  class Connector;
}

namespace media {

class CAPTURE_EXPORT VideoCaptureJpegDecoder;

class CAPTURE_EXPORT VideoCaptureJpegDecoderFactory {
 public:
  using ReceiveDecodedFrameCB = base::Callback<void(
      int buffer_id,
      int frame_feedback_id,
      std::unique_ptr<VideoCaptureDevice::Client::Buffer::
                          ScopedAccessPermission> buffer_read_permission,
      mojom::VideoFrameInfoPtr frame_info)>;

  virtual ~VideoCaptureJpegDecoderFactory() {}

  // |done_cb| guarantees that the required context stays alive during the
  // asynchronous execution. On successful creation of a Jpeg decoder,
  // |decoder_created_cb| is invoked with the result. If creation is
  // unsuccessful, no callback to |decoder_created_cb| is made.
  virtual void CreateJpegDecoderAsync(
      const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_receive_decoded_frame,
      base::Callback<void(std::unique_ptr<VideoCaptureJpegDecoder>)>
          decoder_created_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_decoder_created,
      base::OnceClosure done_cb) = 0;

  static bool IsPlatformSupported();
};

class CAPTURE_EXPORT ServiceConnectorProvider {
 public:
  virtual ~ServiceConnectorProvider() {}
  // May execute synchronously.
  virtual void GetConnectorAsync(
      base::Callback<void(service_manager::Connector*)> callback) = 0;
};

// Operates a given base::WeakPtr<ServiceConnectorProvider> on a given
// SingleThreadTaskRunner.
class CAPTURE_EXPORT ServiceConnectorProviderOnTaskRunner
    : public ServiceConnectorProvider {
 public:
  ServiceConnectorProviderOnTaskRunner(
      base::WeakPtr<ServiceConnectorProvider> provider,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~ServiceConnectorProviderOnTaskRunner() override;
  void GetConnectorAsync(
      base::Callback<void(service_manager::Connector*)> callback) override;

 private:
  base::WeakPtr<ServiceConnectorProvider> provider_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtrFactory<ServiceConnectorProvider> weak_factory_;
};

// Implementation of VideoCaptureJpegDecoderFactory that uses a given
// ServiceConnectorProvider to establish a connection to the GPU process.
class CAPTURE_EXPORT ConnectorProviderBasedVideoCaptureJpegDecoderFactory
    : public VideoCaptureJpegDecoderFactory {
 public:
  // |io_task_runner| gets passed to the constructor of class
  // VideoCaptureGpuJpegDecoder.
  // If present, |optional_ipc_task_runner| gets passed to ui::Gpu::Create().
  ConnectorProviderBasedVideoCaptureJpegDecoderFactory(
    std::unique_ptr<ServiceConnectorProvider> connector_provider,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> optional_ipc_task_runner);
  ~ConnectorProviderBasedVideoCaptureJpegDecoderFactory() override;

  // Implementation of media::VideoCaptureJpegDecoderFactory
  void CreateJpegDecoderAsync(
      const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_receive_decoded_frame,
      base::Callback<void(std::unique_ptr<media::VideoCaptureJpegDecoder>)>
          decoder_created_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_decoder_created,
      base::OnceClosure done_cb) override;

 private:
  void EstablishGpuChannel(
      const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_receive_decoded_frame,
      base::Callback<void(std::unique_ptr<media::VideoCaptureJpegDecoder>)>
          decoder_created_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_decoder_created,
      base::OnceClosure done_cb,
      service_manager::Connector* connector);

  void OnGpuChannelEstablished(
      const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_receive_decoded_frame,
      base::Callback<void(std::unique_ptr<media::VideoCaptureJpegDecoder>)>
          decoder_created_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_decoder_created,
      base::OnceClosure done_cb,
      std::unique_ptr<ui::Gpu> gpu,
      scoped_refptr<gpu::GpuChannelHost> channel_host);

  const std::unique_ptr<ServiceConnectorProvider> connector_provider_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner> optional_ipc_task_runner_;
};

class CAPTURE_EXPORT VideoCaptureJpegDecoder {
 public:
  virtual ~VideoCaptureJpegDecoder() {}

  virtual bool IsInErrorState() const = 0;

  // Decodes a JPEG picture.
  virtual void DecodeCapturedData(
      const uint8_t* data,
      size_t in_buffer_size,
      const media::VideoCaptureFormat& frame_format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      media::VideoCaptureDevice::Client::Buffer out_buffer) = 0;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_JPEG_DECODER_H_
