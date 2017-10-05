// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/hosted_app_button_container.h"

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/button/menu_button_listener.h"
#include "ui/views/layout/box_layout.h"

namespace {

class HostedAppMenuButton : public views::MenuButton,
                            public views::MenuButtonListener {
 public:
  explicit HostedAppMenuButton(BrowserView* browser_view)
      : views::MenuButton(base::string16(), this, false),
        browser_view_(browser_view) {
    SetInkDropMode(InkDropMode::ON);
  }
  ~HostedAppMenuButton() override {}

  void SetUseLightImages(bool use_light) {
    SkColor color = use_light ? SK_ColorWHITE : gfx::kChromeIconGrey;
    SetImage(views::Button::STATE_NORMAL,
             gfx::CreateVectorIcon(kBrowserToolsIcon, color));
    set_ink_drop_base_color(color);
  }

  // views::MenuButtonListener:
  void OnMenuButtonClicked(views::MenuButton* source,
                           const gfx::Point& point,
                           const ui::Event* event) override {
    Browser* browser = browser_view_->browser();
    menu_.reset(new AppMenu(browser, 0));
    menu_model_.reset(new AppMenuModel(browser_view_, browser));
    menu_->Init(menu_model_.get());

    menu_->RunMenu(this);
  }

 private:
  BrowserView* browser_view_;

  // App model and menu.
  // Note that the menu should be destroyed before the model it uses, so the
  // menu should be listed later.
  std::unique_ptr<AppMenuModel> menu_model_;
  std::unique_ptr<AppMenu> menu_;

  DISALLOW_COPY_AND_ASSIGN(HostedAppMenuButton);
};

}  // namespace

HostedAppButtonContainer::HostedAppButtonContainer(BrowserView* browser_view)
    : app_menu_button_(new HostedAppMenuButton(browser_view)) {
  auto* layout = new views::BoxLayout(views::BoxLayout::kHorizontal);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(layout);
  AddChildView(app_menu_button_);
}

HostedAppButtonContainer::~HostedAppButtonContainer() {}

void HostedAppButtonContainer::SetUseLightImages(bool use_light) {
  app_menu_button_->SetUseLightImages(use_light);
}
