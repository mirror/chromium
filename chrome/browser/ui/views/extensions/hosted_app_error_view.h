// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXTENSIONS_HOSTED_APP_ERROR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_EXTENSIONS_HOSTED_APP_ERROR_VIEW_H_

#include "chrome/browser/ui/extensions/hosted_app_error.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget_delegate.h"

namespace content {
class WebContents;
}

namespace views {
class Label;
class LabelButton;
class Widget;
}  // namespace views

class HostedAppErrorView : public extensions::HostedAppError,
                           public views::WidgetDelegate,
                           public views::ButtonListener {
 public:
  HostedAppErrorView(
      content::WebContents* web_contents,
      OnWebContentsReparentedCallback on_web_contents_reparented);
  ~HostedAppErrorView() override;

  // views::WidgetDelegate
  views::View* GetContentsView() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

  void ButtonPressed(views::Button* source, const ui::Event& event) override;

 private:
  views::View* main_contents_;
  views::Widget* widget_;

  views::LabelButton* action_button_;
  views::Label* title_;

  DISALLOW_COPY_AND_ASSIGN(HostedAppErrorView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXTENSIONS_HOSTED_APP_ERROR_VIEW_H_
