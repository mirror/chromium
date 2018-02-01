// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/wired_display/presentation_receiver_window_original_profile.h"

#include "chrome/browser/media/router/providers/wired_display/wired_display_presentation_receiver_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "third_party/WebKit/public/web/WebPresentationReceiverFlags.h"

namespace media_router {
namespace {

using content::WebContents;

WebContents::CreateParams CreateWebContentsParams(
    Profile* profile,
    const std::pair<int, int>& opener_render_frame_id) {
  WebContents::CreateParams params(profile);
  params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;
  if (opener_render_frame_id.first != -1 &&
      opener_render_frame_id.second != -1) {
    scoped_refptr<content::SiteInstance> site_instance(
        content::RenderFrameHost::FromID(opener_render_frame_id.first,
                                         opener_render_frame_id.second)
            ->GetSiteInstance());
    params.site_instance = site_instance;
    params.opener_render_process_id = opener_render_frame_id.first;
    params.opener_render_frame_id = opener_render_frame_id.second;
    params.opener_suppressed = false;
  }
  return params;
}

}  // namespace

PresentationReceiverWindowOriginalProfile::
    PresentationReceiverWindowOriginalProfile(
        Profile* profile,
        const gfx::Rect& bounds,
        const std::pair<int, int>& opener_render_frame_id,
        base::OnceClosure termination_callback,
        base::RepeatingCallback<void(const std::string&)> title_change_callback)
    : web_contents_(WebContents::Create(
          CreateWebContentsParams(profile, opener_render_frame_id))),
      receiver_(WiredDisplayPresentationReceiverFactory::CreateWithWebContents(
          web_contents_.get(),
          bounds,
          std::move(termination_callback),
          std::move(title_change_callback))) {}

PresentationReceiverWindowOriginalProfile::
    ~PresentationReceiverWindowOriginalProfile() = default;

// WiredDisplayPresentationReceiver overrides.
void PresentationReceiverWindowOriginalProfile::Start(
    const std::string& presentation_id,
    const GURL& start_url) {
  receiver_->Start(presentation_id, start_url);
}

void PresentationReceiverWindowOriginalProfile::Terminate() {
  receiver_->Terminate();
}

content::WebContents* PresentationReceiverWindowOriginalProfile::web_contents()
    const {
  return web_contents_.get();
}

}  // namespace media_router
