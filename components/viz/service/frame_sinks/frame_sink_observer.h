// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef dasfrqwe
#define dasfrqwe

namespace viz {

class FrameSinkId;
class CompositorFrame;

class FrameSinkObserver {
 public:
  virtual ~FrameSinkObserver() {}
  virtual void OnCompositorFrameReceived(const FrameSinkId& frame_sink_id,
                                         const CompositorFrame& frame) = 0;
};

}  // namespace viz

#endif  // dasfrqwe
