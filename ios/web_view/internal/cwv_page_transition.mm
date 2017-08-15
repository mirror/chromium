// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#import "ios/web_view/public/cwv_page_transition.h"
#import "ios/web_view/internal/cwv_page_transition_internal.h"

#include "base/logging.h"

CWVPageTransition CWVPageTransitionFromUIPageTransition(
    ui::PageTransition ui_page_transition) {
  CWVPageTransition cwv_page_transition = 0;

  ui::PageTransition core_transition =
      ui::PageTransitionStripQualifier(ui_page_transition);
  switch (core_transition) {
    case ui::PAGE_TRANSITION_LINK:
      cwv_page_transition = CWVPageTransitionLink;
      break;
    case ui::PAGE_TRANSITION_TYPED:
      cwv_page_transition = CWVPageTransitionTyped;
      break;
    case ui::PAGE_TRANSITION_AUTO_BOOKMARK:
      cwv_page_transition = CWVPageTransitionAutoBookmark;
      break;
    case ui::PAGE_TRANSITION_AUTO_SUBFRAME:
      cwv_page_transition = CWVPageTransitionAutoSubframe;
      break;
    case ui::PAGE_TRANSITION_MANUAL_SUBFRAME:
      cwv_page_transition = CWVPageTransitionManualSubframe;
      break;
    case ui::PAGE_TRANSITION_GENERATED:
      cwv_page_transition = CWVPageTransitionGenerated;
      break;
    case ui::PAGE_TRANSITION_AUTO_TOPLEVEL:
      cwv_page_transition = CWVPageTransitionAutoToplevel;
      break;
    case ui::PAGE_TRANSITION_FORM_SUBMIT:
      cwv_page_transition = CWVPageTransitionFormSubmit;
      break;
    case ui::PAGE_TRANSITION_RELOAD:
      cwv_page_transition = CWVPageTransitionReload;
      break;
    case ui::PAGE_TRANSITION_KEYWORD:
      cwv_page_transition = CWVPageTransitionKeyword;
      break;
    case ui::PAGE_TRANSITION_KEYWORD_GENERATED:
      cwv_page_transition = CWVPageTransitionKeywordGenerated;
      break;
    default:
      // We cannot rely on the compiler to perform this check because
      // ui::PageTransition contains both core values and qualifiers while only
      // core values are enumerated here.
      NOTREACHED() << "Unknown core value of ui::PageTransition. Update "
                      "CWVPageTransitionFromUIPageTransition() to add one.";
  }

  if (ui_page_transition & ui::PAGE_TRANSITION_BLOCKED) {
    cwv_page_transition |= CWVPageTransitionBlocked;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_FORWARD_BACK) {
    cwv_page_transition |= CWVPageTransitionForwardBack;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_FROM_ADDRESS_BAR) {
    cwv_page_transition |= CWVPageTransitionFromAddressBar;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_HOME_PAGE) {
    cwv_page_transition |= CWVPageTransitionHomePage;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_FROM_API) {
    cwv_page_transition |= CWVPageTransitionFromApi;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_CHAIN_START) {
    cwv_page_transition |= CWVPageTransitionChainStart;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_CHAIN_END) {
    cwv_page_transition |= CWVPageTransitionChainEnd;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_CLIENT_REDIRECT) {
    cwv_page_transition |= CWVPageTransitionClientRedirect;
  }
  if (ui_page_transition & ui::PAGE_TRANSITION_SERVER_REDIRECT) {
    cwv_page_transition |= CWVPageTransitionServerRedirect;
  }

  return cwv_page_transition;
}
