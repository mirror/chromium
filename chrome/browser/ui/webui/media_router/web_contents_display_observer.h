// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_AURA_H_
#define CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_AURA_H_

#include "base/callback.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget_observer.h"

namespace display {
class Display;
}

namespace content {
class WebContents;
}

namespace media_router {

class WebContentsDisplayObserver : public BrowserListObserver,
                                   public views::WidgetObserver {
 public:
  WebContentsDisplayObserver(content::WebContents* web_contents,
                             base::RepeatingCallback<void()> callback);

  ~WebContentsDisplayObserver() override;

  int64_t GetDisplayId() const;

  // BrowserListObserver overrides:
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserAdded(Browser* browser) override;

  // views::WidgetObserver overrides:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

 private:
  void UpdateWidget();

  // The display that |window_| is on.
  display::Display display_;

  content::WebContents* web_contents_;

  views::Widget* widget_;

  // Called when the display that |window_| is on changes.
  base::RepeatingCallback<void()> callback_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_AURA_H_
