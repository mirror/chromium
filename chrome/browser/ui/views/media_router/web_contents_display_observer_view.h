// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_VIEW_H_

#include "base/callback.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/webui/media_router/web_contents_display_observer.h"
#include "ui/display/display.h"
#include "ui/views/widget/widget_observer.h"

namespace media_router {

class WebContentsDisplayObserverView : public WebContentsDisplayObserver,
                                       public BrowserListObserver,
                                       public views::WidgetObserver {
 public:
  WebContentsDisplayObserverView(content::WebContents* web_contents,
                                 base::RepeatingCallback<void()> callback);

  ~WebContentsDisplayObserverView() override;

  // BrowserListObserver overrides:
  void OnBrowserSetLastActive(Browser* browser) override;

  // views::WidgetObserver overrides:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  const display::Display& GetDisplay() const override;

 private:
  content::WebContents* web_contents_;

  // The widget containing |web_contents_|.
  views::Widget* widget_;

  // The display that |web_contents_| is on.
  display::Display display_;

  // Called when the display that |web_contents_| is on changes.
  base::RepeatingCallback<void()> callback_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_WEB_CONTENTS_DISPLAY_OBSERVER_VIEW_H_
