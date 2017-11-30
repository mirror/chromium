// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_PHOTO_CALLBACK_FACTORY_MF_WIN_H_
#define MEDIA_CAPTURE_VIDEO_WIN_PHOTO_CALLBACK_FACTORY_MF_WIN_H_

#include <mfcaptureengine.h>

#include "media/capture/video/video_capture_device.h"

namespace media {

// Photo callback factory producing MediaFoundation
// IMFCaptureEngineOnSampleCallback based on a TakePhotoCallback and a
// VideoCaptureFormat
class CAPTURE_EXPORT PhotoCallbackFactoryMFWin {
 public:
  PhotoCallbackFactoryMFWin();

  virtual ~PhotoCallbackFactoryMFWin();

  virtual scoped_refptr<IMFCaptureEngineOnSampleCallback> CreateCallback(
      VideoCaptureDevice::TakePhotoCallback callback,
      VideoCaptureFormat format);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_PHOTO_CALLBACK_FACTORY_MF_WIN_H_