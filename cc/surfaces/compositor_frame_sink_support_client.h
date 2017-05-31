// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SURFACES_COMPOSITOR_FRAME_SINK_SUPPORT_CLIENT_H_
#define CC_SURFACES_COMPOSITOR_FRAME_SINK_SUPPORT_CLIENT_H_

#include "base/time/time.h"
#include "cc/resources/returned_resource.h"

namespace gfx {
class Rect;
}

namespace cc {

class LocalSurfaceId;

struct BeginFrameArgs;

class CompositorFrameSinkSupportClient {
 public:
  // Notification that the previous CompositorFrame given to
  // SubmitCompositorFrame() has been processed and that another frame
  // can be submitted. This provides backpressure from the display compositor
  // so that frames are submitted only at the rate it can handle them.
  // TODO(fsamuel): This method ought not be necessary with unified BeginFrame.
  // However, there's a fair amount of cleanup and refactoring necessary to get
  // rid of it.
  virtual void DidReceiveCompositorFrameAck(
      const ReturnedResourceArray& resources) = 0;

  // Notification that the frame with |presentation_token| has been presented to
  // user. The |timestamp| corresponds to the time when the content updat turned
  // into light the first time on the display. The |refresh| is prediction of
  // how long the next output refresh may occur.
  // Note: If the |presentation_token| is zero, |DidPresentCompositorFrame| and
  // |DidDiscardCompositorFrame| will never be called.
  virtual void DidPresentCompositorFrame(uint32_t presentation_token,
                                         base::TimeTicks timestamp,
                                         base::TimeDelta refresh) = 0;

  // Notification that the frame with |presentation_token| has been discarded.
  // The content of the frame was never displayed to the user.
  // Note: If the |presentation_token| is zero, |DidPresentCompositorFrame| and
  // |DidDiscardCompositorFrame| will never be called.
  virtual void DidDiscardCompositorFrame(uint32_t presentation_token) = 0;

  // Notification for the client to generate a CompositorFrame.
  virtual void OnBeginFrame(const BeginFrameArgs& args) = 0;

  // Returns resources sent to SubmitCompositorFrame to be reused or freed.
  virtual void ReclaimResources(const ReturnedResourceArray& resources) = 0;

  // Called when surface is being scheduled for a draw.
  virtual void WillDrawSurface(const LocalSurfaceId& local_surface_id,
                               const gfx::Rect& damage_rect) = 0;

 protected:
  virtual ~CompositorFrameSinkSupportClient() {}
};

}  // namespace cc

#endif  // CC_SURFACES_COMPOSITOR_FRAME_SINK_SUPPORT_CLIENT_H_
