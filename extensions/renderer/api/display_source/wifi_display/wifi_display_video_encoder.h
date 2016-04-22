// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_VIDEO_ENCODER_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_VIDEO_ENCODER_H_

#include "base/memory/shared_memory.h"
#include "extensions/renderer/api/display_source/wifi_display/wifi_display_media_encoder.h"
#include "media/base/video_frame.h"
#include "media/video/video_encode_accelerator.h"
#include "third_party/wds/src/libwds/public/video_format.h"

namespace extensions {

using WiFiDisplayEncodedFrame = WiFiDisplayEncodedUnit;

// This interface represents H.264 video encoder used by the
// Wi-Fi Display media pipeline.
// Threading: the client code should belong to a single thread.
class WiFiDisplayVideoEncoder : public WiFiDisplayMediaEncoder {
 public:
  using VideoEncoderCallback =
      base::Callback<void(scoped_refptr<WiFiDisplayVideoEncoder>)>;

  using ReceiveVideoEncodeAcceleratorCallback =
      base::Callback<void(scoped_refptr<base::SingleThreadTaskRunner>,
                          std::unique_ptr<media::VideoEncodeAccelerator>)>;
  using CreateVideoEncodeAcceleratorCallback =
      base::Callback<void(const ReceiveVideoEncodeAcceleratorCallback&)>;

  using ReceiveEncodeMemoryCallback =
      base::Callback<void(std::unique_ptr<base::SharedMemory>)>;
  using CreateEncodeMemoryCallback =
      base::Callback<void(size_t size, const ReceiveEncodeMemoryCallback&)>;

  struct InitParameters {
    InitParameters();
    InitParameters(const InitParameters&);
    ~InitParameters();
    gfx::Size frame_size;
    int frame_rate;
    int bit_rate;
    wds::H264Profile profile;
    wds::H264Level level;
    // VEA-specific parameters.
    CreateEncodeMemoryCallback create_memory_callback;
    CreateVideoEncodeAcceleratorCallback vea_create_callback;
  };

  // A factory method that creates a new encoder instance from the given
  // |params|, the encoder instance is returned as an argument of
  // |result_callback| ('nullptr' argument means encoder creation failure).
  static void Create(const InitParameters& params,
                     const VideoEncoderCallback& encoder_callback);

  // Encodes the given raw frame. The resulting encoded frame is passed
  // as an |encoded_callback|'s argument which is set via 'SetCallbacks'
  // method.
  void InsertRawVideoFrame(const scoped_refptr<media::VideoFrame>& video_frame,
                           base::TimeTicks reference_time);

  // Requests the next encoded frame to be an instantaneous decoding refresh
  // (IDR) picture.
  void RequestIDRPicture();

 protected:
  explicit WiFiDisplayVideoEncoder(
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner);
  ~WiFiDisplayVideoEncoder() override;

  virtual void InsertFrameOnMediaThread(
      scoped_refptr<media::VideoFrame> video_frame,
      base::TimeTicks reference_time,
      bool send_idr) = 0;

  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  bool send_idr_;
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_VIDEO_ENCODER_H_
