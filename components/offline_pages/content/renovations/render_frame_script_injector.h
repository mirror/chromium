// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CONTENT_RENDER_FRAME_SCRIPT_INJECTOR_H_
#define COMPONENTS_OFFLINE_PAGES_CONTENT_RENDER_FRAME_SCRIPT_INJECTOR_H_

#include "components/offline_pages/core/renovations/script_injector.h"
#include "content/public/browser/render_frame_host.h"

namespace offline_pages {

// ScriptInjector for running scripts in a RenderFrame within a given isolated
// world id.
class RenderFrameScriptInjector : public ScriptInjector {
 public:
  RenderFrameScriptInjector(content::RenderFrameHost* render_frame_host,
                            int isolated_world_id);

  void Inject(base::string16 script, ResultCallback callback) override;

 private:
  content::RenderFrameHost* render_frame_host_;
  int isolated_world_id_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CONTENT_RENDER_FRAME_SCRIPT_INJECTOR_H_
