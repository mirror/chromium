// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/insecure_content_infobar_delegate.h"

#include "base/metrics/histogram.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/render_messages.h"
#include "content/common/page_transition_types.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

InsecureContentInfoBarDelegate::InsecureContentInfoBarDelegate(
    TabContentsWrapper* tab_contents,
    InfoBarType type)
    : ConfirmInfoBarDelegate(tab_contents->tab_contents()),
      tab_contents_(tab_contents),
      type_(type) {
  UMA_HISTOGRAM_ENUMERATION("InsecureContentInfoBarDelegate",
      (type_ == DISPLAY) ? DISPLAY_INFOBAR_SHOWN : RUN_INFOBAR_SHOWN,
      NUM_EVENTS);
}

InsecureContentInfoBarDelegate::~InsecureContentInfoBarDelegate() {
}

void InsecureContentInfoBarDelegate::InfoBarDismissed() {
  UMA_HISTOGRAM_ENUMERATION("InsecureContentInfoBarDelegate",
      (type_ == DISPLAY) ? DISPLAY_INFOBAR_DISMISSED : RUN_INFOBAR_DISMISSED,
      NUM_EVENTS);
  ConfirmInfoBarDelegate::InfoBarDismissed();
}

InsecureContentInfoBarDelegate*
    InsecureContentInfoBarDelegate::AsInsecureContentInfoBarDelegate() {
  return this;
}

string16 InsecureContentInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16((type_ == DISPLAY) ?
      IDS_BLOCKED_DISPLAYING_INSECURE_CONTENT :
      IDS_BLOCKED_RUNNING_INSECURE_CONTENT);
}

int InsecureContentInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

string16 InsecureContentInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  DCHECK_EQ(button, BUTTON_OK);
  return l10n_util::GetStringUTF16(IDS_ALLOW_INSECURE_CONTENT_BUTTON);
}

bool InsecureContentInfoBarDelegate::Accept() {
  UMA_HISTOGRAM_ENUMERATION("InsecureContentInfoBarDelegate",
      (type_ == DISPLAY) ? DISPLAY_USER_OVERRIDE : RUN_USER_OVERRIDE,
      NUM_EVENTS);

  int32 routing_id = tab_contents_->routing_id();
  tab_contents_->Send((type_ == DISPLAY) ? static_cast<IPC::Message*>(
      new ViewMsg_SetAllowDisplayingInsecureContent(routing_id, true)) :
      new ViewMsg_SetAllowRunningInsecureContent(routing_id, true));
  return true;
}

string16 InsecureContentInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

bool InsecureContentInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  tab_contents_->tab_contents()->OpenURL(
      GURL((type_ == DISPLAY) ?
          "https://www.google.com/support/chrome/bin/answer.py?answer=1342710" :
          "https://www.google.com/support/chrome/bin/answer.py?answer=1342714"),
      GURL(), (disposition == CURRENT_TAB) ? NEW_FOREGROUND_TAB : disposition,
      PageTransition::LINK);
  return false;
}
