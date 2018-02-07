// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_
#define CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "cc/cc_export.h"
#include "cc/trees/render_frame_metadata.h"

namespace cc {

// Observes RenderFrameMetadata associated with the submission of a frame.
// LayerTreeHostImpl will create the metadata during DrawLayers. This is only
// notified when enabled().
//
// Calls to this will be done from the compositor thread.
class CC_EXPORT RenderFrameMetadataObserver {
 public:
  RenderFrameMetadataObserver() = default;
  virtual ~RenderFrameMetadataObserver() = default;

  // Synchronous notification of the RendarFrameMetadata for the frame being
  // submitted to the display compositor.
  virtual void OnRenderFrameSubmission(RenderFrameMetadata* metadata) = 0;

  // Provides implementations with a task runner that runs on the compositor
  // thread. Allows for posting tasks to follow up on IPC requests which arrive
  // on the IO thread.
  virtual void SetCompositorTaskRunner(
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner) = 0;

  bool enabled() const { return enabled_; }

 protected:
  void SetEnabled(bool enabled) { enabled_ = enabled; }

 private:
  // When true the LayerTreeHostImpl will notify of each frame submission.
  bool enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameMetadataObserver);
};

}  // namespace cc

#endif  // CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_
