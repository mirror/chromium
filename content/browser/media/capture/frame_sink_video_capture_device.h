// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_FRAME_SINK_VIDEO_CAPTURE_DEVICE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_FRAME_SINK_VIDEO_CAPTURE_DEVICE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "content/browser/media/capture/cursor_renderer.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "media/capture/video/video_capture_device.h"
#include "media/capture/video/video_frame_receiver.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_video_capture.mojom.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

class CursorRenderer;

// A virtualized VideoCaptureDevice that captures the displayed contents of a
// frame sink (see viz::CompositorFrameSink), such as the composited main view
// of a WebContents instance, producing a stream of video frames.
//
// From the point-of-view of the VIZ service, this is a consumer of video frames
// (viz::mojom::FrameSinkVideoConsumer). However, from the point-of-view of the
// video capture stack, this is a device (media::VideoCaptureDevice) that
// produces video frames. Therefore, a FrameSinkVideoCaptureDevice is really a
// proxy between the two subsystems.
//
// Usually, a subclass implementation is instantiated and used, such as
// WebContentsVideoCaptureDevice or AuraWindowCaptureDevice. These subclasses
// provide additional implementation, to update which frame sink is targeted for
// capture, and to notify other components that capture is taking place.
class CONTENT_EXPORT FrameSinkVideoCaptureDevice
    : public media::VideoCaptureDevice,
      public viz::mojom::FrameSinkVideoConsumer {
 public:
  FrameSinkVideoCaptureDevice();
  ~FrameSinkVideoCaptureDevice() override;

  // Must only be dereferenced on the device thread.
  base::WeakPtr<FrameSinkVideoCaptureDevice> GetWeakPtr();

  // Deviation from the VideoCaptureDevice interface: Since the memory pooling
  // provided by a VideoCaptureDevice::Client is not needed, this
  // FrameSinkVideoCaptureDevice will provide frames to a VideoFrameReceiver
  // directly.
  void AllocateAndStart(const media::VideoCaptureParams& params,
                        std::unique_ptr<media::VideoFrameReceiver> receiver);

  // VideoCaptureDevice implementation.
  void AllocateAndStart(const media::VideoCaptureParams& params,
                        std::unique_ptr<Client> client) final;
  void RequestRefreshFrame() final;
  void MaybeSuspend() final;
  void Resume() final;
  void StopAndDeAllocate() final;
  void OnUtilizationReport(int frame_feedback_id, double utilization) final;

  // FrameSinkVideoConsumer implementation.
  void OnFrameCaptured(mojo::ScopedSharedBufferHandle buffer, uint32_t buffer_size, media::mojom::VideoFrameInfoPtr info, const gfx::Rect& update_rect, const gfx::Rect& content_rect, viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) final;
  void OnTargetLost(const viz::FrameSinkId& frame_sink_id) final;
  void OnStopped() final;

  // These are called to notify when the capture target has changed or was
  // permanently lost. |frame_sink_id| specifies a target compositor frame sink,
  // while |native_view| is the local UI to monitor for the puproses of mouse
  // cursor rendering.
  void OnTargetChanged(const viz::FrameSinkId& frame_sink_id, gfx::NativeView native_view);
  void OnTargetPermanentlyLost();

 protected:
  // Called by FrameSinkVideoCaptureDevice to notify subclasses when capture has
  // started/stopped.
  virtual void WillStart(const media::VideoCaptureParams& params);
  virtual void DidStop();

 private:
  using BufferId = decltype(media::VideoCaptureDevice::Client::Buffer::id);

  // Helper that logs the given error |message| to the |receiver_| and then
  // stops capture.
  void OnFatalError(const std::string& message);

  // Current capture target. This is cached to resolve a race where
  // OnTargetChanged() can be called before the |capturer_| is created in
  // AllocateAndStart().
  viz::FrameSinkId target_;

  // Receives video frames from this capture device, for propagation to render
  // frames. This is set by AllocateAndStart(), and cleared by
  // StopAndDeAllocate().
  std::unique_ptr<media::VideoFrameReceiver> receiver_;

  // Mojo pointer to the capturer instance in VIZ.
  viz::mojom::FrameSinkVideoCapturerPtr capturer_;

  // Mojo binding to this instance as a consumer of frames from the capturer.
  mojo::Binding<viz::mojom::FrameSinkVideoConsumer> binding_;

  // A pool of structs that hold state relevant to frames currently being
  // consumed by VideoFrameReceiver. Each "slot" is re-used by later frames.
  struct ConsumptionState {
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks;
    CursorRendererUndoer undoer;

    ConsumptionState();
    ~ConsumptionState();

    ConsumptionState(ConsumptionState&& other);
    ConsumptionState& operator=(ConsumptionState&& other);
  };
  std::vector<ConsumptionState> slots_;

  // Set when OnFatalError() is called. This prevents any future
  // AllocateAndStart() calls from succeeding.
  base::Optional<std::string> fatal_error_message_;

  SEQUENCE_CHECKER(sequence_checker_);

  // Renders the mouse cursor on each video frame.
  const std::unique_ptr<CursorRenderer, BrowserThread::DeleteOnUIThread> cursor_renderer_;

  // Creates WeakPtrs for use on the device thread.
  base::WeakPtrFactory<FrameSinkVideoCaptureDevice> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkVideoCaptureDevice);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_FRAME_SINK_VIDEO_CAPTURE_DEVICE_H_
