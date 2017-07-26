// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/content/renovations/render_frame_script_injector.h"

#include <utility>

#include "base/logging.h"
#include "base/values.h"
#include "content/public/common/isolated_world_ids.h"

namespace offline_pages {

RenderFrameScriptInjector::RenderFrameScriptInjector(
    content::RenderFrameHost* render_frame_host,
    int isolated_world_id)
    : render_frame_host_(render_frame_host),
      isolated_world_id_(isolated_world_id) {
  DCHECK(render_frame_host_);
  DCHECK(isolated_world_id_ > content::ISOLATED_WORLD_ID_GLOBAL &&
         isolated_world_id_ < content::ISOLATED_WORLD_ID_MAX);
}

void RenderFrameScriptInjector::Inject(base::string16 script,
                                       ResultCallback callback) {
  render_frame_host_->ExecuteJavaScriptInIsolatedWorld(
      script, std::move(callback), isolated_world_id_);
}

}  // namespace offline_pages
