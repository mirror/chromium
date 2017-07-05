// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_ANDROID_OVERLAY_WEB_CONTENTS_OBSERVER_H_
#define CONTENT_BROWSER_ANDROID_ANDROID_OVERLAY_WEB_CONTENTS_OBSERVER_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "content/browser/android/dialog_overlay_impl.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class CONTENT_EXPORT AndroidOverlayWebContentsObserver
    : public WebContentsObserver {
 public:
  class Client {
   public:
    virtual void RemoveObserverForWebContents(WebContents* web_contents) = 0;
    virtual void OverlayVisibilityChanged() = 0;
  };

  AndroidOverlayWebContentsObserver(WebContents* web_contents,
                                    Client* client,
                                    bool is_initially_visible);
  ~AndroidOverlayWebContentsObserver() override;

  bool ShouldUseOverlayMode();

  void AddOverlay(RenderFrameHost* render_frame_host,
                  DialogOverlayImpl* overlay);
  void RemoveOverlay(RenderFrameHost* render_frame_host,
                     DialogOverlayImpl* overlay);

  // WebContentsObserver
  void WasShown() override;
  void WasHidden() override;
  void WebContentsDestroyed() override;
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(RenderFrameHost* old_host,
                              RenderFrameHost* new_host) override;

 private:
  void StopOverlaysInFrame(RenderFrameHost* render_frame_host);

  void SignalChangeOrRemoval();

  Client* client_;
  bool web_contents_is_visible_;

  base::flat_map<RenderFrameHost*, base::flat_set<DialogOverlayImpl*>>
      live_overlays_map_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_ANDROID_OVERLAY_WEB_CONTENTS_OBSERVER_H_
