// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef IOS_WEB_VIEW_PUBLIC_CWV_PAGE_TRANSITION_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_PAGE_TRANSITION_H_

#import <Foundation/Foundation.h>

// Types of transitions between pages.
//
// This is a mirror of ui::PageTransition. It is mirrored here because
// //ios/web_view API must be in pure Objective-C, and not allowed to include
// C++ headers like //ui/base/page_transition_types.h.
typedef NS_OPTIONS(NSUInteger, CWVPageTransition) {
  // User got to this page by clicking a link on another page.
  CWVPageTransitionLink = 0,

  // User got this page by typing the URL in the URL bar.  This should not be
  // used for cases where the user selected a choice that didn't look at all
  // like a URL; see GENERATED below.
  //
  // We also use this for other "explicit" navigation actions.
  CWVPageTransitionTyped = 1,

  // User got to this page through a suggestion in the UI, for example)
  // through the destinations page.
  CWVPageTransitionAutoBookmark = 2,

  // This is a subframe navigation. This is any content that is automatically
  // loaded in a non-toplevel frame. For example, if a page consists of
  // several frames containing ads, those ad URLs will have this transition
  // type. The user may not even realize the content in these pages is a
  // separate frame, so may not care about the URL (see MANUAL below).
  CWVPageTransitionAutoSubframe = 3,

  // For subframe navigations that are explicitly requested by the user and
  // generate new navigation entries in the back/forward list. These are
  // probably more important than frames that were automatically loaded in
  // the background because the user probably cares about the fact that this
  // link was loaded.
  CWVPageTransitionManualSubframe = 4,

  // User got to this page by typing in the URL bar and selecting an entry
  // that did not look like a URL.  For example, a match might have the URL
  // of a Google search result page, but appear like "Search Google for ...".
  // These are not quite the same as TYPED navigations because the user
  // didn't type or see the destination URL.
  // See also KEYWORD.
  CWVPageTransitionGenerated = 5,

  // This is a toplevel navigation. This is any content that is automatically
  // loaded in a toplevel frame.  For example, opening a tab to show the ASH
  // screen saver, opening the devtools window, opening the NTP after the safe
  // browsing warning, opening web-based dialog boxes are examples of
  // AUTO_TOPLEVEL navigations.
  CWVPageTransitionAutoToplevel = 6,

  // The user filled out values in a form and submitted it. NOTE that in
  // some situations submitting a form does not result in this transition
  // type. This can happen if the form uses script to submit the contents.
  CWVPageTransitionFormSubmit = 7,

  // The user "reloaded" the page, either by hitting the reload button or by
  // hitting enter in the address bar.  NOTE: This is distinct from the
  // concept of whether a particular load uses "reload semantics" (i.e.
  // bypasses cached data).  For this reason, lots of code needs to pass
  // around the concept of whether a load should be treated as a "reload"
  // separately from their tracking of this transition type, which is mainly
  // used for proper scoring for consumers who care about how frequently a
  // user typed/visited a particular URL.
  //
  // SessionRestore and undo tab close use this transition type too.
  CWVPageTransitionReload = 8,

  // The url was generated from a replaceable keyword other than the default
  // search provider. If the user types a keyword (which also applies to
  // tab-to-search) in the omnibox this qualifier is applied to the transition
  // type of the generated url. TemplateURLModel then may generate an
  // additional visit with a transition type of KEYWORD_GENERATED against the
  // url 'http://' + keyword. For example, if you do a tab-to-search against
  // wikipedia the generated url has a transition qualifer of KEYWORD, and
  // TemplateURLModel generates a visit for 'wikipedia.org' with a transition
  // type of KEYWORD_GENERATED.
  CWVPageTransitionKeyword = 9,

  // Corresponds to a visit generated for a keyword. See description of
  // KEYWORD for more details.
  CWVPageTransitionKeywordGenerated = 10,

  // ADDING NEW CORE VALUE? Be sure to update ui::PageTransition too.
  CWVPageTransitionLastCore = CWVPageTransitionKeywordGenerated,
  CWVPageTransitionCoreMask = 0xFF,

  // Qualifiers
  // Any of the core values above can be augmented by one or more qualifiers.
  // These qualifiers further define the transition.

  // A managed user attempted to visit a URL but was blocked.
  CWVPageTransitionBlocked = 0x00800000,

  // User used the Forward or Back button to navigate among browsing history.
  CWVPageTransitionForwardBack = 0x01000000,

  // User used the address bar to trigger this navigation.
  CWVPageTransitionFromAddressBar = 0x02000000,

  // User is navigating to the home page.
  CWVPageTransitionHomePage = 0x04000000,

  // The transition originated from an external application; the exact
  // definition of this is embedder dependent.
  CWVPageTransitionFromApi = 0x08000000,

  // The beginning of a navigation chain.
  CWVPageTransitionChainStart = 0x10000000,

  // The last transition in a redirect chain.
  CWVPageTransitionChainEnd = 0x20000000,

  // Redirects caused by JavaScript or a meta refresh tag on the page.
  CWVPageTransitionClientRedirect = 0x40000000,

  // Redirects sent from the server by HTTP headers. It might be nice to
  // break this out into 2 types in the future, permanent or temporary, if we
  // can get that information from WebKit.
  CWVPageTransitionServerRedirect = 0x80000000,

  // Used to test whether a transition involves a redirect.
  CWVPageTransitionIsRedirectMask = 0xC0000000,

  // General mask defining the bits used for the qualifiers.
  CWVPageTransitionQualifierMask = 0xFFFFFF00,
};

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_PAGE_TRANSITION_H_
