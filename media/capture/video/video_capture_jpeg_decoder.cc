// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/video_capture_jpeg_decoder.h"

#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "media/base/media_switches.h"
#include "media/capture/video/video_capture_gpu_jpeg_decoder.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "services/ui/public/interfaces/constants.mojom.h"

namespace media {

// static
bool VideoCaptureJpegDecoderFactory::IsPlatformSupported() {
  bool is_platform_supported =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseFakeJpegDecodeAccelerator);
#if defined(OS_CHROMEOS)
  is_platform_supported = true;
#endif

  if (!is_platform_supported ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableAcceleratedMjpegDecode)) {
    LOG(ERROR) << "Initializing gpu jpeg decoder aborted, because "
                  "unsupported platform.";
    UMA_HISTOGRAM_BOOLEAN("Media.VideoCaptureGpuJpegDecoder.InitDecodeSuccess",
                          false);
    return false;
  }
  return true;
}

ServiceConnectorProviderOnTaskRunner::ServiceConnectorProviderOnTaskRunner(
    base::WeakPtr<ServiceConnectorProvider> provider,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : provider_(std::move(provider)),
      task_runner_(std::move(task_runner)),
      weak_factory_(this) {}

ServiceConnectorProviderOnTaskRunner::~ServiceConnectorProviderOnTaskRunner() =
    default;

void ServiceConnectorProviderOnTaskRunner::GetConnectorAsync(
    base::Callback<void(service_manager::Connector*)> callback) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&ServiceConnectorProvider::GetConnectorAsync,
                              weak_factory_.GetWeakPtr(), callback));
    return;
  }
  if (!provider_)
    return;
  provider_->GetConnectorAsync(callback);
}

ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
    ConnectorProviderBasedVideoCaptureJpegDecoderFactory(
        std::unique_ptr<ServiceConnectorProvider> connector_provider,
        scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
        scoped_refptr<base::SingleThreadTaskRunner> optional_ipc_task_runner)
    : connector_provider_(std::move(connector_provider)),
      io_task_runner_(std::move(io_task_runner)),
      optional_ipc_task_runner_(std::move(optional_ipc_task_runner)) {}

ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
    ~ConnectorProviderBasedVideoCaptureJpegDecoderFactory() = default;

void ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
    CreateJpegDecoderAsync(
        const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
        scoped_refptr<base::SingleThreadTaskRunner>
            task_runner_for_receive_decoded_frame,
        base::Callback<void(std::unique_ptr<media::VideoCaptureJpegDecoder>)>
            decoder_created_cb,
        scoped_refptr<base::SingleThreadTaskRunner>
            task_runner_for_decoder_created,
        base::OnceClosure done_cb) {
  if (!VideoCaptureJpegDecoderFactory::IsPlatformSupported())
    return;

  connector_provider_->GetConnectorAsync(
      base::Bind(&ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
                     EstablishGpuChannel,
                 base::Unretained(this), receive_decoded_frame_cb,
                 std::move(task_runner_for_receive_decoded_frame),
                 decoder_created_cb, std::move(task_runner_for_decoder_created),
                 base::Passed(&done_cb)));
}

void ConnectorProviderBasedVideoCaptureJpegDecoderFactory::EstablishGpuChannel(
    const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
    scoped_refptr<base::SingleThreadTaskRunner>
        task_runner_for_receive_decoded_frame,
    base::Callback<void(std::unique_ptr<media::VideoCaptureJpegDecoder>)>
        decoder_created_cb,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_for_decoder_created,
    base::OnceClosure done_cb,
    service_manager::Connector* connector) {
  if (!connector)
    return;

  // mojom::kBrowserServiceName,
  // ui::mojom::kServiceName,
  auto gpu = ui::Gpu::Create(connector, "content_browser",
                             optional_ipc_task_runner_);
  gpu->EstablishGpuChannel(
      base::Bind(&ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
                     OnGpuChannelEstablished,
                 base::Unretained(this), receive_decoded_frame_cb,
                 std::move(task_runner_for_receive_decoded_frame),
                 decoder_created_cb, std::move(task_runner_for_decoder_created),
                 base::Passed(&done_cb), base::Passed(&gpu)));
}

void ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
    OnGpuChannelEstablished(
        const ReceiveDecodedFrameCB& receive_decoded_frame_cb,
        scoped_refptr<base::SingleThreadTaskRunner>
            task_runner_for_receive_decoded_frame,
        base::Callback<void(std::unique_ptr<media::VideoCaptureJpegDecoder>)>
            decoder_created_cb,
        scoped_refptr<base::SingleThreadTaskRunner>
            task_runner_for_decoder_created,
        base::OnceClosure done_cb,
        std::unique_ptr<ui::Gpu> gpu,
        scoped_refptr<gpu::GpuChannelHost> channel_host) {
  // Hop to task_runner_for_decoder_created thread.
  if (!task_runner_for_decoder_created->BelongsToCurrentThread()) {
    task_runner_for_decoder_created->PostTask(
        FROM_HERE,
        base::Bind(&ConnectorProviderBasedVideoCaptureJpegDecoderFactory::
                       OnGpuChannelEstablished,
                   base::Unretained(this), receive_decoded_frame_cb,
                   std::move(task_runner_for_receive_decoded_frame),
                   decoder_created_cb,
                   std::move(task_runner_for_decoder_created),
                   base::Passed(&done_cb),
                   base::Passed(&gpu),
                   base::Passed(&channel_host)));
    return;
  }
  // |gpu| must outlive |channel_host|. Therefore, we pass ownership of
  // |gpu| to the newly created VideoCaptureGpuJpegDecoder instance.
  decoder_created_cb.Run(base::MakeUnique<media::VideoCaptureGpuJpegDecoder>(
      base::Bind([](std::unique_ptr<ui::Gpu>) {}, base::Passed(&gpu)),
      std::move(channel_host), io_task_runner_, receive_decoded_frame_cb,
      std::move(task_runner_for_receive_decoded_frame)));
}

}  // namespace media
