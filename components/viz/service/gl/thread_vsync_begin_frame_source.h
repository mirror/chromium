// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_GPU_VSYNC_BEGIN_FRAME_SOURCE_H_
#define CONTENT_BROWSER_COMPOSITOR_GPU_VSYNC_BEGIN_FRAME_SOURCE_H_

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "gpu/ipc/common/surface_handle.h"

namespace viz {

class GpuVSyncWorker;

// This class is used to control VSync production on GPU side.
class ThreadVSyncControl {
 public:
  virtual void SetNeedsVSync(bool needs_vsync) = 0;
};

// This is a type of ExternalBeginFrameSource where VSync signals are
// generated externally on GPU side.
class ThreadVSyncBeginFrameSource : public ExternalBeginFrameSource,
                                    public ExternalBeginFrameSourceClient {
 public:
  ThreadVSyncBeginFrameSource(ThreadVSyncControl* vsync_control,
                              gpu::SurfaceHandle surface_handle);
  ~ThreadVSyncBeginFrameSource() override;

  // ExternalBeginFrameSourceClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  void OnVSync(base::TimeTicks timestamp, base::TimeDelta interval);

 protected:
  // Virtual for testing.
  virtual base::TimeTicks Now() const;

 private:
  BeginFrameArgs GetMissedBeginFrameArgs(
      BeginFrameObserver* obs) override;

  // VSync worker that waits for VSync events on worker thread.
  scoped_refptr<GpuVSyncWorker> vsync_worker_;

  ThreadVSyncControl* const vsync_control_;
  bool needs_begin_frames_;
  uint64_t next_sequence_number_;

  DISALLOW_COPY_AND_ASSIGN(ThreadVSyncBeginFrameSource);
};

}  // namespace viz

#endif  // CONTENT_BROWSER_COMPOSITOR_GPU_VSYNC_BEGIN_FRAME_SOURCE_H_
