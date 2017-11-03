// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_HOSTED_APP_ERROR_H_
#define CHROME_BROWSER_UI_EXTENSIONS_HOSTED_APP_ERROR_H_

#include <memory>

#include "base/bind_helpers.h"
#include "base/callback_forward.h"
#include "base/macros.h"

namespace content {
class WebContents;
}

namespace extensions {

// Cross-platform interface to show a Hosted App Error overlay.  If
// the user clicks the proceed button on the page, this class will reparent
// the WebContents it's shown on into to a regular browser window, creating a
// new one if necessary. Once the WebContents is reparented runs its
// OnWebContentsReparentedCallback.
class HostedAppError {
 public:
  using OnWebContentsReparentedCallback = base::OnceClosure;

  // Creates a new HostedAppError page and shows it on top of |web_contents|.
  // |on_web_contents_reparented| is run when |web_contents| is reparented. See
  // the class' description.
  static std::unique_ptr<HostedAppError> Create(
      content::WebContents* web_contents,
      OnWebContentsReparentedCallback on_web_contents_reparented);

  virtual ~HostedAppError();

  void Proceed();

  content::WebContents* web_contents() { return web_contents_; }

 protected:
  HostedAppError(content::WebContents* web_contents,
                 OnWebContentsReparentedCallback on_web_contents_reparented);

 private:
  content::WebContents* web_contents_;
  OnWebContentsReparentedCallback on_web_contents_reparented_;

  DISALLOW_COPY_AND_ASSIGN(HostedAppError);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_UI_EXTENSIONS_HOSTED_APP_ERROR_H_
