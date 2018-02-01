// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/wired_display/presentation_receiver_window_otr_profile.h"

#include "chrome/browser/media/router/providers/wired_display/wired_display_presentation_receiver_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "third_party/WebKit/public/web/WebPresentationReceiverFlags.h"

namespace media_router {
namespace {

using content::WebContents;

WebContents::CreateParams CreateWebContentsParams(Profile* profile) {
  WebContents::CreateParams params(profile);
  params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;
  return params;
}

}  // namespace

PresentationReceiverWindowOTRProfile::PresentationReceiverWindowOTRProfile(
    Profile* profile,
    const gfx::Rect& bounds,
    base::OnceClosure termination_callback,
    base::RepeatingCallback<void(const std::string&)> title_change_callback)
    : otr_profile_registration_(
          IndependentOTRProfileManager::GetInstance()
              ->CreateFromOriginalProfile(
                  profile,
                  base::BindOnce(&PresentationReceiverWindowOTRProfile::
                                     OriginalProfileDestroyed,
                                 base::Unretained(this)))),
      web_contents_(WebContents::Create(
          CreateWebContentsParams(otr_profile_registration_->profile()))),
      receiver_(WiredDisplayPresentationReceiverFactory::CreateWithWebContents(
          web_contents_.get(),
          bounds,
          std::move(termination_callback),
          std::move(title_change_callback))) {}

PresentationReceiverWindowOTRProfile::~PresentationReceiverWindowOTRProfile() =
    default;

void PresentationReceiverWindowOTRProfile::Start(
    const std::string& presentation_id,
    const GURL& start_url) {
  receiver_->Start(presentation_id, start_url);
}

void PresentationReceiverWindowOTRProfile::Terminate() {
  receiver_->Terminate();
}

content::WebContents* PresentationReceiverWindowOTRProfile::web_contents()
    const {
  return web_contents_.get();
}

void PresentationReceiverWindowOTRProfile::OriginalProfileDestroyed(
    Profile* profile) {
  otr_profile_registration_.reset();
  web_contents_.reset();
  Terminate();
}

}  // namespace media_router
