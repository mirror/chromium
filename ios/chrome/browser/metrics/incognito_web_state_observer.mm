// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/metrics/incognito_web_state_observer.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_list.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IncognitoWebStateObserver::IncognitoWebStateObserver() {
  TabModelList::AddObserver(this);
}

IncognitoWebStateObserver::~IncognitoWebStateObserver() {
  TabModelList::RemoveObserver(this);
}

void IncognitoWebStateObserver::TabModelRegisteredWithBrowserState(
    TabModel* tab_model,
    ios::ChromeBrowserState* browser_state) {
  if (browser_state->IsOffTheRecord())
    [tab_model webStateList]->AddObserver(this);
}

void IncognitoWebStateObserver::TabModelUnregisteredFromBrowserState(
    TabModel* tab_model,
    ios::ChromeBrowserState* browser_state) {
  if (browser_state->IsOffTheRecord())
    [tab_model webStateList]->RemoveObserver(this);
}

void IncognitoWebStateObserver::WebStateInsertedAt(WebStateList* web_state_list,
                                                   web::WebState* web_state,
                                                   int index,
                                                   bool activating) {
  OnIncognitoWebStateAdded();
}

void IncognitoWebStateObserver::WebStateDetachedAt(WebStateList* web_state_list,
                                                   web::WebState* web_state,
                                                   int index) {
  OnIncognitoWebStateRemoved();
}

void IncognitoWebStateObserver::OnIncognitoWebStateAdded() {}

void IncognitoWebStateObserver::OnIncognitoWebStateRemoved() {}
