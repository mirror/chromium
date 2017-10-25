// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BOOKMARK_APP_ERROR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_BOOKMARK_APP_ERROR_VIEW_H_

#include "chrome/browser/ui/bookmark_app_error.h"

#include "ui/views/controls/button/button.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}

namespace views {
class Label;
class LabelButton;
}

class BookmarkAppErrorView : public chrome::BookmarkAppError,
                             public views::View,
                             public views::ButtonListener {
 public:
  BookmarkAppErrorView(content::WebContents* web_contents);
  ~BookmarkAppErrorView() override;

  // views::ButtonListener
  void ButtonPressed(views::Button* source, const ui::Event& event) override;

 private:
  views::Label* title_;
  views::Label* empty_;
  views::LabelButton* action_button_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkAppErrorView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BOOKMARK_APP_ERROR_VIEW_H_
