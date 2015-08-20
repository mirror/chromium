// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MANDOLINE_UI_BROWSER_DESKTOP_DESKTOP_UI_H_
#define MANDOLINE_UI_BROWSER_DESKTOP_DESKTOP_UI_H_

#include "mandoline/ui/aura/aura_init.h"
#include "mandoline/ui/browser/browser.h"
#include "mandoline/ui/browser/browser_ui.h"
#include "mandoline/ui/browser/public/interfaces/omnibox.mojom.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/layout_manager.h"

namespace mojo {
class ApplicationConnection;
class Shell;
class View;
}

namespace views {
class LabelButton;
}

namespace mandoline {

class Browser;
class ProgressView;

class DesktopUI : public BrowserUI,
                  public views::LayoutManager,
                  public views::ButtonListener {
 public:
  DesktopUI(mojo::ApplicationImpl* application_impl, BrowserManager* manager);
  ~DesktopUI() override;

 private:
  // Overridden from BrowserUI
  void Init(mojo::View* root) override;
  void LoadURL(const GURL& url) override;
  void ViewManagerDisconnected() override;
  void EmbedOmnibox(mojo::ApplicationConnection* connection) override;
  void OnURLChanged() override;
  void LoadingStateChanged(bool loading) override;
  void ProgressChanged(double progress) override;

  // Overridden from views::LayoutManager:
  gfx::Size GetPreferredSize(const views::View* view) const override;
  void Layout(views::View* host) override;

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  void ShowOmnibox();

  mojo::ApplicationImpl* application_impl_;
  Browser browser_;
  BrowserManager* manager_;
  scoped_ptr<AuraInit> aura_init_;
  views::LabelButton* omnibox_launcher_;
  ProgressView* progress_bar_;
  mojo::View* root_;
  mojo::View* content_;
  mojo::View* omnibox_;

  DISALLOW_COPY_AND_ASSIGN(DesktopUI);
};

}  // namespace mandoline

#endif  // MANDOLINE_UI_BROWSER_DESKTOP_DESKTOP_UI_H_
