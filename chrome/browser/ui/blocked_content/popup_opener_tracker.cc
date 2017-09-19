// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/popup_opener_tracker.h"

#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/ui/blocked_content/popup_tracker.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PopupOpenerTracker);

PopupOpenerTracker::PopupOpenerTracker(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), weak_factory_(this) {}

PopupOpenerTracker::~PopupOpenerTracker() {
  if (last_popup_)
    last_popup_->OnOpenerGoingAway();
}

void PopupOpenerTracker::CloseAsync() {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&PopupOpenerTracker::CloseContents,
                                weak_factory_.GetWeakPtr()));
}

void PopupOpenerTracker::RemovePopup(PopupTracker* tracker) {
  if (tracker == last_popup_)
    last_popup_ = nullptr;
}

void PopupOpenerTracker::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (last_popup_)
    last_popup_->OnDidFinishNavigationInOpener(handle);
}

void PopupOpenerTracker::CloseContents() {
  web_contents()->Close();
}
