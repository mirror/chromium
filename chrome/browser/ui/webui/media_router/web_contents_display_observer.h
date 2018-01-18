// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_AURA_H_
#define CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_AURA_H_

#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tree_host.h"
#include "ui/aura/window_tree_host_observer.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace display {
class Display;
}

namespace media_router {

class WebContentsDisplayObserver : public BrowserListObserver,
                                   public aura::WindowTreeHostObserver,
                                   public aura::WindowObserver {
 public:
  WebContentsDisplayObserver(content::WebContents* web_contents,
                             base::RepeatingCallback<void()> callback);

  ~WebContentsDisplayObserver() override;

  // aura::WindowTreeHostObserver overrides:
  void OnHostMovedInPixels(aura::WindowTreeHost* host,
                           const gfx::Point& new_origin_in_pixels) override;

  int64_t GetDisplayId() const;

  // BrowserListObserver overrides:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserAdded(Browser* browser) override;

  // aura::WindowObserver overrides:
  void OnWindowDestroying(aura::Window* window) override;

 private:
  void UpdateWindow();

  // The display that |window_| is on.
  display::Display display_;

  content::WebContents* web_contents_;

  gfx::NativeWindow window_;

  // Called when the display that |window_| is on changes.
  base::RepeatingCallback<void()> callback_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_AURA_H_
