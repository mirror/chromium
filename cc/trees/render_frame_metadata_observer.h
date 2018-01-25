// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_
#define CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "cc/cc_export.h"
#include "cc/trees/render_frame_metadata.h"

namespace cc {

class CC_EXPORT RenderFrameMetadataObserver
    : public base::RefCountedThreadSafe<RenderFrameMetadataObserver> {
 public:
  RenderFrameMetadataObserver() {}

  virtual void OnRenderFrameSubmission(RenderFrameMetadata* metadata) = 0;

 protected:
  friend class base::RefCountedThreadSafe<RenderFrameMetadataObserver>;
  virtual ~RenderFrameMetadataObserver() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderFrameMetadataObserver);
};

}  // namespace cc

#endif  // CC_TREES_RENDER_FRAME_METADATA_OBSERVER_H_
