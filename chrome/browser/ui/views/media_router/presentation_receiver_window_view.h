// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/command_updater_delegate.h"
#include "chrome/browser/ui/toolbar/chrome_toolbar_model_delegate.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/views/widget/widget_delegate.h"

class ToolbarModelImpl;

// This class implements the View portion of PresentationReceiverWindow.  It
// contains a WebView for displaying the receiver page and a LocationBarView for
// displaying the URL.
class PresentationReceiverWindowView final : public views::WidgetDelegateView,
                                             public LocationBarView::Delegate,
                                             public CommandUpdaterDelegate,
                                             public ChromeToolbarModelDelegate {
 public:
  class Controller {
   public:
    virtual void WindowClosing() = 0;
    virtual content::WebContents* GetWebContents() const = 0;

   protected:
    ~Controller() = default;
  };

  explicit PresentationReceiverWindowView(Controller* controller);
  ~PresentationReceiverWindowView() final;

  void Init();

  // Notifies us that the navigation state of the underlying WebContents has
  // changed so the LocationBarView may need to be updated.
  void UpdateLocationBar();

  void set_title(const base::string16& title) { title_ = title; }

 private:
  // LocationBarView::Delegate overrides.
  content::WebContents* GetWebContents() final;
  ToolbarModel* GetToolbarModel() final;
  const ToolbarModel* GetToolbarModel() const final;
  ContentSettingBubbleModelDelegate* GetContentSettingBubbleModelDelegate()
      final;

  // CommandUpdaterDelegate overrides.
  void ExecuteCommandWithDisposition(int id,
                                     WindowOpenDisposition disposition) final;

  // ChromeToolbarModelDelegate overrides.
  content::WebContents* GetActiveWebContents() const final;

  // views::WidgetDelegateView overrides.
  void WindowClosing() final;
  base::string16 GetWindowTitle() const final;

  Controller* const controller_;
  base::string16 title_;
  const std::unique_ptr<ToolbarModelImpl> toolbar_model_;
  CommandUpdater command_updater_;
  LocationBarView* location_bar_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PresentationReceiverWindowView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_VIEW_H_
