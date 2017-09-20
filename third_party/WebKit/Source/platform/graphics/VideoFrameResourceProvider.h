// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VideoFrameResourceProvider_h
#define VideoFrameResourceProvider_h

#include "cc/resources/video_resource_updater.h"

namespace viz {
class ContextProvider;
class RenderPass;
}

namespace blink {

// Placeholder class, to be implemented in full in later CL.
// VideoFrameResourceProvider obtains required GPU resources for the video
// frame.
class VideoFrameResourceProvider {
 public:
  explicit VideoFrameResourceProvider(
      const base::Callback<viz::ContextProvider*()>&);

  void AppendQuads(viz::RenderPass&);

 private:
  const base::Callback<viz::ContextProvider*()>& context_provider_callback_;
  cc::VideoResourceUpdater resource_updater_;
};

}  // namespace blink

#endif  // VideoFrameResourceProvider_h
