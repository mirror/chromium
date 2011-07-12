// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/infobar_delegate.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "content/browser/tab_contents/navigation_details.h"
#include "content/browser/tab_contents/navigation_entry.h"
#include "content/browser/tab_contents/tab_contents.h"

// InfoBarDelegate ------------------------------------------------------------

InfoBarDelegate::~InfoBarDelegate() {
}

bool InfoBarDelegate::EqualsDelegate(InfoBarDelegate* delegate) const {
  return false;
}

bool InfoBarDelegate::ShouldExpire(
    const content::LoadCommittedDetails& details) const {
  if (!details.is_navigation_to_different_page())
    return false;

  return ShouldExpireInternal(details);
}

bool InfoBarDelegate::ShouldExpireInternal(
    const content::LoadCommittedDetails& details) const {
  return (contents_unique_id_ != details.entry->unique_id()) ||
      (PageTransition::StripQualifier(details.entry->transition_type()) ==
          PageTransition::RELOAD);
}

void InfoBarDelegate::InfoBarDismissed() {
}

void InfoBarDelegate::InfoBarClosed() {
  delete this;
}

gfx::Image* InfoBarDelegate::GetIcon() const {
  return NULL;
}

InfoBarDelegate::Type InfoBarDelegate::GetInfoBarType() const {
  return WARNING_TYPE;
}

ConfirmInfoBarDelegate* InfoBarDelegate::AsConfirmInfoBarDelegate() {
  return NULL;
}

ExtensionInfoBarDelegate* InfoBarDelegate::AsExtensionInfoBarDelegate() {
  return NULL;
}

InsecureContentInfoBarDelegate*
    InfoBarDelegate::AsInsecureContentInfoBarDelegate() {
  return NULL;
}

LinkInfoBarDelegate* InfoBarDelegate::AsLinkInfoBarDelegate() {
  return NULL;
}

PluginInstallerInfoBarDelegate*
    InfoBarDelegate::AsPluginInstallerInfoBarDelegate() {
  return NULL;
}

ThemeInstalledInfoBarDelegate*
    InfoBarDelegate::AsThemePreviewInfobarDelegate() {
  return NULL;
}

TranslateInfoBarDelegate* InfoBarDelegate::AsTranslateInfoBarDelegate() {
  return NULL;
}

InfoBarDelegate::InfoBarDelegate(TabContents* contents)
    : contents_unique_id_(0) {
  if (contents)
    StoreActiveEntryUniqueID(contents);
}

void InfoBarDelegate::StoreActiveEntryUniqueID(TabContents* contents) {
  NavigationEntry* active_entry = contents->controller().GetActiveEntry();
  contents_unique_id_ = active_entry ? active_entry->unique_id() : 0;
}
