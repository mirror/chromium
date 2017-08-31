// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/picture_in_picture/picture_in_picture_window_controller.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "chrome/browser/ui/overlay/overlay_window.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PictureInPictureWindowController);

//static
PictureInPictureWindowController* PictureInPictureWindowController::GetOrCreateForWebContents(content::WebContents* web_contents) {
  DCHECK(web_contents);
  LOG(ERROR) << "GetOrCreateForWebContents: " << web_contents->GetURL();

  // This is a no-op if the controller already exists.
  // PictureInPictureWindowController::CreateForWebContents(web_contents);
  // return PictureInPictureWindowController::FromWebContents(web_contents);
  CreateForWebContents(web_contents);
  return FromWebContents(web_contents);
}

PictureInPictureWindowController::~PictureInPictureWindowController() {
  window_->Close();
}

PictureInPictureWindowController::PictureInPictureWindowController(content::WebContents* initiator)
: initiator_(initiator),
  window_(OverlayWindow::Create()) {
  DCHECK(initiator_);
  window_->Init();
}

void PictureInPictureWindowController::Show() {
  DCHECK(window_);
  window_->Show();
}

bool PictureInPictureWindowController::IsShowingPictureInPictureWindow() {
  return window_ && window_->IsActive();
}
