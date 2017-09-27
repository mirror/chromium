// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VideoFrameResourceProvider_h
#define VideoFrameResourceProvider_h

#include "cc/resources/video_resource_updater.h"
#include "media/base/context_provider_callback.h"

namespace viz {
class RenderPass;
}

namespace blink {

// Placeholder class, to be implemented in full in later CL.
// VideoFrameResourceProvider obtains required GPU resources for the video
// frame.
class VideoFrameResourceProvider {
 public:
  explicit VideoFrameResourceProvider(const media::ContextProviderCallback&);

  void Initialize(viz::ContextProvider*);

  void AppendQuads(viz::RenderPass&);

  base::WeakPtr<VideoFrameResourceProvider> CreateWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<VideoFrameResourceProvider> weak_ptr_factory_;
  media::ContextProviderCallback context_provider_callback_;
  std::unique_ptr<cc::VideoResourceUpdater> resource_updater_;
};

}  // namespace blink

#endif  // VideoFrameResourceProvider_h
