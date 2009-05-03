// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_DEFAULT_BROWSER_INFOBAR_H_
#define CHROME_BROWSER_VIEWS_DEFAULT_BROWSER_INFOBAR_H_

#include "chrome/browser/views/info_bar_confirm_view.h"

class Profile;

// DefaultBrowserInfoBar provides a convenient and non-modal way to make
// Chrome the default browser.
class DefaultBrowserInfoBar : public InfoBarConfirmView {
 public:
  explicit DefaultBrowserInfoBar(Profile* profile);
  virtual ~DefaultBrowserInfoBar();

  // Overridden from InfoBarConfirmView:
  virtual void OKButtonPressed();
  virtual void CancelButtonPressed();

 private:
  Profile* profile_;

  // Whether or not the user explicitly clicked on one of our two buttons.
  bool action_taken_;

  DISALLOW_COPY_AND_ASSIGN(DefaultBrowserInfoBar);
};


#endif  // #ifndef CHROME_BROWSER_VIEWS_DEFAULT_BROWSER_INFOBAR_H_
