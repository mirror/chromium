// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Photo callback factory producing MediaFoundation
// IMFCaptureEngineOnSampleCallback.

#include <mfcaptureengine.h>

#include "media/capture/video/video_capture_device.h"

namespace media {

class CAPTURE_EXPORT MFPhotoCallbackFactory {
 public:
  MFPhotoCallbackFactory();

  ~MFPhotoCallbackFactory();

  std::unique_ptr<IMFCaptureEngineOnSampleCallback> build(
      media::VideoCaptureDevice::TakePhotoCallback callback,
      VideoCaptureFormat format);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace media