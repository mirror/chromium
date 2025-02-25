// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_SURFACE_UTILS_H_
#define CONTENT_BROWSER_COMPOSITOR_SURFACE_UTILS_H_

#include <memory>

#include "components/viz/common/surfaces/frame_sink_id.h"
#include "content/common/content_export.h"
#include "content/public/browser/readback_types.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "ui/gfx/geometry/size.h"

namespace viz {
class CopyOutputResult;
class FrameSinkManagerImpl;
class HostFrameSinkManager;
}

namespace content {

CONTENT_EXPORT viz::FrameSinkId AllocateFrameSinkId();

CONTENT_EXPORT viz::FrameSinkManagerImpl* GetFrameSinkManager();

CONTENT_EXPORT viz::HostFrameSinkManager* GetHostFrameSinkManager();

void CopyFromCompositingSurfaceHasResult(
    const gfx::Size& dst_size_in_pixel,
    const SkColorType color_type,
    const ReadbackRequestCallback& callback,
    std::unique_ptr<viz::CopyOutputResult> result);

namespace surface_utils {

// Directly connects HostFrameSinkManager to FrameSinkManagerImpl without Mojo.
CONTENT_EXPORT void ConnectWithLocalFrameSinkManager(
    viz::HostFrameSinkManager* host_frame_sink_manager,
    viz::FrameSinkManagerImpl* frame_sink_manager_impl);

}  // namespace surface_utils

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_SURFACE_UTILS_H_
