// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PICTURE_IN_PICTURE_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_
#define CHROME_BROWSER_PICTURE_IN_PICTURE_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_

// #include <memory>
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class OverlayWindow;

// Write details here.
class PictureInPictureWindowController : public content::WebContentsUserData<PictureInPictureWindowController> {
 public:
  ~PictureInPictureWindowController() override;
  // Gets a reference to the controller associated with |web_contents| and creates one if it does not exist. The returned pointer is guaranteed to be non-null.
  static PictureInPictureWindowController* GetOrCreateForWebContents(content::WebContents* initiator);

  void Show();

  // Indicates if the window already exists.
  bool IsShowingPictureInPictureWindow();

 protected:
  friend class content::WebContentsUserData<PictureInPictureWindowController>;

  // Use PictureInPictureWindowController::GetOrCreateForWebContents() to
  // create an instance.
  explicit PictureInPictureWindowController(content::WebContents* initiator);
  

 private:
  content::WebContents* const initiator_;
  std::unique_ptr<OverlayWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(PictureInPictureWindowController);
};

#endif  // CHROME_BROWSER_PICTURE_IN_PICTURE_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_