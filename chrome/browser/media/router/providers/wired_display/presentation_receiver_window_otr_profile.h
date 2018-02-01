// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_WIRED_DISPLAY_PRESENTATION_RECEIVER_WINDOW_OTR_PROFILE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_WIRED_DISPLAY_PRESENTATION_RECEIVER_WINDOW_OTR_PROFILE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "chrome/browser/media/router/presentation/independent_otr_profile_manager.h"
#include "chrome/browser/media/router/providers/wired_display/wired_display_presentation_receiver.h"

class GURL;
class Profile;

namespace content {
class WebContents;
}

namespace gfx {
class Rect;
}

namespace media_router {

class PresentationReceiverWindowOTRProfile final
    : public WiredDisplayPresentationReceiver {
 public:
  PresentationReceiverWindowOTRProfile(
      Profile* profile,
      const gfx::Rect& bounds,
      base::OnceClosure termination_callback,
      base::RepeatingCallback<void(const std::string&)> title_change_callback);
  ~PresentationReceiverWindowOTRProfile() final;

  // WiredDisplayPresentationReceiver overrides.
  void Start(const std::string& presentation_id, const GURL& start_url) final;
  void Terminate() final;
  content::WebContents* web_contents() const final;

 private:
  void OriginalProfileDestroyed(Profile* profile);

  std::unique_ptr<IndependentOTRProfileManager::OTRProfileRegistration>
      otr_profile_registration_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<WiredDisplayPresentationReceiver> receiver_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_WIRED_DISPLAY_PRESENTATION_RECEIVER_WINDOW_OTR_PROFILE_H_
