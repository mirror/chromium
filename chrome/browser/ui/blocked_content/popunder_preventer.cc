// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/popunder_preventer.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "extensions/features/features.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "components/guest_view/browser/guest_view_base.h"
#endif

PopunderPreventer::PopunderPreventer(content::WebContents* activating_contents)
    : popup_(nullptr) {
  content::WebContents* actual_host = activating_contents;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // If the dialog was triggered via an PDF, get the actual web contents that
  // embeds the PDF.
  guest_view::GuestViewBase* guest =
      guest_view::GuestViewBase::FromWebContents(activating_contents);
  if (guest)
    actual_host = guest->embedder_web_contents();
#endif

  // If the WebContents that triggered this dialog is not currently focused, we
  // want to store a potential popup here to restore it after the dialog was
  // closed.
  Browser* active_browser = BrowserList::GetInstance()->GetLastActive();
  if (active_browser) {
    content::WebContents* active_web_contents =
        active_browser->tab_strip_model()->GetActiveWebContents();
    if (active_browser->is_type_popup() && active_web_contents) {
      content::WebContents* original_opener =
          content::WebContents::FromRenderFrameHost(
              active_web_contents->GetOriginalOpener());
      if (original_opener == actual_host) {
        // It's indeed a popup from the dialog opening WebContents. Store it, so
        // we can focus it later.
        popup_ = active_web_contents;
        Observe(popup_);
      }
    }
  }
}

PopunderPreventer::~PopunderPreventer() {
  if (!popup_)
    return;

  content::WebContentsDelegate* delegate = popup_->GetDelegate();
  if (!delegate)
    return;

  delegate->ActivateContents(popup_);
}

void PopunderPreventer::WebContentsDestroyed() {
  popup_ = nullptr;
}
