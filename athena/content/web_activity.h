// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATHENA_CONTENT_PUBLIC_WEB_ACTIVITY_H_
#define ATHENA_CONTENT_PUBLIC_WEB_ACTIVITY_H_

#include <vector>

#include "athena/activity/public/activity.h"
#include "athena/activity/public/activity_view_model.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/image/image_skia.h"

class SkBitmap;

namespace content {
class BrowserContext;
class WebContents;
}

namespace gfx {
class Size;
}

namespace views {
class WebView;
class WidgetDelegate;
}

namespace athena {

class AthenaWebView;
class ContentProxy;

class WebActivity : public Activity,
                    public ActivityViewModel,
                    public content::WebContentsObserver {
 public:
  WebActivity(content::BrowserContext* context,
              const base::string16& title,
              const GURL& gurl);
  explicit WebActivity(content::WebContents* contents);

 protected:
  virtual ~WebActivity();

 // Activity:
  virtual athena::ActivityViewModel* GetActivityViewModel() override;
  virtual void SetCurrentState(ActivityState state) override;
  virtual ActivityState GetCurrentState() override;
  virtual bool IsVisible() override;
  virtual ActivityMediaState GetMediaState() override;
  virtual aura::Window* GetWindow() override;
  virtual content::WebContents* GetWebContents() override;

  // ActivityViewModel:
  virtual void Init() override;
  virtual SkColor GetRepresentativeColor() const override;
  virtual base::string16 GetTitle() const override;
  virtual gfx::ImageSkia GetIcon() const override;
  virtual bool UsesFrame() const override;
  virtual views::View* GetContentsView() override;
  virtual views::Widget* CreateWidget() override;
  virtual gfx::ImageSkia GetOverviewModeImage() override;
  virtual void PrepareContentsForOverview() override;
  virtual void ResetContentsView() override;

  // content::WebContentsObserver:
  virtual void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  virtual void TitleWasSet(content::NavigationEntry* entry,
                           bool explicit_set) override;
  virtual void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& candidates) override;
  virtual void DidChangeThemeColor(SkColor theme_color) override;

 private:
  // Called when a favicon download initiated in DidUpdateFaviconURL()
  // has completed.
  void OnDidDownloadFavicon(
      int id,
      int http_status_code,
      const GURL& url,
      const std::vector<SkBitmap>& bitmaps,
      const std::vector<gfx::Size>& original_bitmap_sizes);

  // Hiding the contet proxy and showing the real content instead.
  void HideContentProxy();

  // Showing a content proxy instead of the real content to save resoruces.
  void ShowContentProxy();

  content::BrowserContext* browser_context_;
  AthenaWebView* web_view_;
  const base::string16 title_;
  SkColor title_color_;
  gfx::ImageSkia icon_;

  // The current state for this activity.
  ActivityState current_state_;

  // The content proxy.
  scoped_ptr<ContentProxy> content_proxy_;

  base::WeakPtrFactory<WebActivity> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebActivity);
};

}  // namespace athena

#endif  // ATHENA_CONTENT_WEB_ACTIVITY_H_
