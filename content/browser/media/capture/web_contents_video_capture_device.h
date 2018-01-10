// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_WEB_CONTENTS_VIDEO_CAPTURE_DEVICE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_WEB_CONTENTS_VIDEO_CAPTURE_DEVICE_H_

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/media/capture/frame_sink_video_capture_device.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"

namespace content {

// Captures the displayed contents of a WebContents, producing a stream of video
// frames.
//
// Generally, Create() is called with a device ID string that contains
// information necessary for finding a WebContents instance. Thereafter, this
// capture device will capture from the frame sink corresponding to the main
// frame of the RenderFrameHost tree for that WebContents instance. As the
// RenderFrameHost tree mutates (e.g., due to page navigations, or crashes/
// reloads), capture will continue without interruption.
class CONTENT_EXPORT WebContentsVideoCaptureDevice
    : public FrameSinkVideoCaptureDevice {
 public:
  WebContentsVideoCaptureDevice(int render_process_id, int main_render_frame_id);

  ~WebContentsVideoCaptureDevice() override;

  // Creates a WebContentsVideoCaptureDevice instance from the given
  // |device_id|. Returns null if |device_id| is invalid.
  static std::unique_ptr<WebContentsVideoCaptureDevice> Create(
      const std::string& device_id);

 private:
  class Targeter;

  // FrameSinkVideoCaptureDevice overrides: These increment/decrement the
  // WebContents's capturer count.
  void WillStart(const media::VideoCaptureParams& params) final;
  void DidStop() final;

  // A helper that runs on the UI thread to monitor changes to the
  // RenderFrameHost tree during the lifetime of a WebContents instance, and
  // posts notifications back to update the target frame sink.
  const std::unique_ptr<Targeter, BrowserThread::DeleteOnUIThread> targeter_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsVideoCaptureDevice);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_WEB_CONTENTS_VIDEO_CAPTURE_DEVICE_H_
