// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_CRW_TEST_WEB_STATE_OBSERVER_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_CRW_TEST_WEB_STATE_OBSERVER_H_

#include "ios/web/public/test/fakes/test_web_state_observer_util.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"

// Test implementation of CRWWebStateObserver protocol.
@interface CRWTestWebStateObserver : NSObject<CRWWebStateObserver>

// Arguments passed to |webStateWasShown:|.
@property(nonatomic, readonly)
    web::TestWasShownInfo* NS_RETURNS_INNER_POINTER wasShownInfo;
// Arguments passed to |webStateWasHidden:|.
@property(nonatomic, readonly)
    web::TestWasHiddenInfo* NS_RETURNS_INNER_POINTER wasHiddenInfo;
// Arguments passed to |webState:didPruneNavigationItemsWithCount:|.
@property(nonatomic, readonly)
    web::TestNavigationItemsPrunedInfo* NS_RETURNS_INNER_POINTER
        navigationItemsPrunedInfo;
// Arguments passed to |webState:didStartNavigation:|.
@property(nonatomic, readonly)
    web::TestDidStartNavigationInfo* NS_RETURNS_INNER_POINTER
        didStartNavigationInfo;
// Arguments passed to |webState:didFinishNavigation:|.
@property(nonatomic, readonly)
    web::TestDidFinishNavigationInfo* NS_RETURNS_INNER_POINTER
        didFinishNavigationInfo;
// Arguments passed to |webState:didCommitNavigationWithDetails:|.
@property(nonatomic, readonly)
    web::TestCommitNavigationInfo* NS_RETURNS_INNER_POINTER
        commitNavigationInfo;
// Arguments passed to |webState:didLoadPageWithSuccess:|.
@property(nonatomic, readonly)
    web::TestLoadPageInfo* NS_RETURNS_INNER_POINTER loadPageInfo;
// Arguments passed to |webState:didChangeLoadingProgress:|.
@property(nonatomic, readonly)
    web::TestChangeLoadingProgressInfo* NS_RETURNS_INNER_POINTER
        changeLoadingProgressInfo;
// Arguments passed to |webStateDidChangeTitle:|.
@property(nonatomic, readonly)
    web::TestTitleWasSetInfo* NS_RETURNS_INNER_POINTER titleWasSetInfo;
// Arguments passed to |webStateDidChangeVisibleSecurityState:|.
@property(nonatomic, readonly)
    web::TestDidChangeVisibleSecurityStateInfo* NS_RETURNS_INNER_POINTER
        didChangeVisibleSecurityStateInfo;
// Arguments passed to |webStateDidSuppressDialog:|.
@property(nonatomic, readonly)
    web::TestDidSuppressDialogInfo* NS_RETURNS_INNER_POINTER
        didSuppressDialogInfo;
// Arguments passed to |webState:didSubmitDocumentWithFormNamed:userInitiated:|.
@property(nonatomic, readonly)
    web::TestSubmitDocumentInfo* NS_RETURNS_INNER_POINTER submitDocumentInfo;
// Arguments passed to
// |webState:didRegisterFormActivity:|.
@property(nonatomic, readonly)
    web::TestFormActivityInfo* NS_RETURNS_INNER_POINTER formActivityInfo;
// Arguments passed to |webState:didUpdateFaviconURLCandidates|.
@property(nonatomic, readonly)
    web::TestUpdateFaviconUrlCandidatesInfo* NS_RETURNS_INNER_POINTER
        updateFaviconUrlCandidatesInfo;
// Arguments passed to |webState:renderProcessGoneForWebState:|.
@property(nonatomic, readonly)
    web::TestRenderProcessGoneInfo* NS_RETURNS_INNER_POINTER
        renderProcessGoneInfo;
// Arguments passed to |webStateDestroyed:|.
@property(nonatomic, readonly)
    web::TestWebStateDestroyedInfo* NS_RETURNS_INNER_POINTER
        webStateDestroyedInfo;
// Arguments passed to |webStateDidStopLoading:|.
@property(nonatomic, readonly)
    web::TestStopLoadingInfo* NS_RETURNS_INNER_POINTER stopLoadingInfo;
// Arguments passed to |webStateDidStartLoading:|.
@property(nonatomic, readonly)
    web::TestStartLoadingInfo* NS_RETURNS_INNER_POINTER startLoadingInfo;

@end

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_CRW_TEST_WEB_STATE_OBSERVER_H_
