// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_list.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_list_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const char kTabModelListKey = 0;

}  // namespace

TabModelList::TabModelList() : tab_models_([[NSMutableSet alloc] init]) {}

TabModelList::~TabModelList() {
  // TabModelList is destroyed during base::SupportsUserData destruction. At
  // that point, all TabModels must have been unregistered already.
  DCHECK_EQ([tab_models_ count], 0u);

  for (auto& observer : observers_)
    observer.TabModelListDestroyed();
}

// static
TabModelList* TabModelList::GetForBrowserState(
    ios::ChromeBrowserState* browser_state,
    bool create) {
  TabModelList* tab_model_list =
      static_cast<TabModelList*>(browser_state->GetUserData(&kTabModelListKey));
  if (!tab_model_list && create) {
    tab_model_list = new TabModelList;
    browser_state->SetUserData(&kTabModelListKey,
                               base::WrapUnique(tab_model_list));
  }
  return tab_model_list;
}

// static
void TabModelList::AddObserver(TabModelListObserver* observer) {
  std::vector<ios::ChromeBrowserState*> browser_states =
      GetApplicationContext()
          ->GetChromeBrowserStateManager()
          ->GetLoadedBrowserStates();

  for (ios::ChromeBrowserState* browser_state : browser_states) {
    DCHECK(!browser_state->IsOffTheRecord());
    TabModelList::GetForBrowserState(browser_state, true)
        ->observers_.AddObserver(observer);
  }
}

// static
void TabModelList::RemoveObserver(TabModelListObserver* observer) {
  std::vector<ios::ChromeBrowserState*> browser_states =
      GetApplicationContext()
          ->GetChromeBrowserStateManager()
          ->GetLoadedBrowserStates();

  for (ios::ChromeBrowserState* browser_state : browser_states) {
    DCHECK(!browser_state->IsOffTheRecord());
    TabModelList::GetForBrowserState(browser_state, true)
        ->observers_.RemoveObserver(observer);
  }
}

// static
void TabModelList::RegisterTabModelWithChromeBrowserState(
    ios::ChromeBrowserState* browser_state,
    TabModel* tab_model) {
  NSMutableSet<TabModel*>* tab_models =
      TabModelList::GetForBrowserState(browser_state, true)->tab_models();
  DCHECK(![tab_models containsObject:tab_model]);
  [tab_models addObject:tab_model];

  TabModelList* original_tab_model_list = TabModelList::GetForBrowserState(
      browser_state->GetOriginalChromeBrowserState(), true);
  for (auto& observer : original_tab_model_list->observers_)
    observer.TabModelRegisteredWithBrowserState(tab_model, browser_state);
}

// static
void TabModelList::UnregisterTabModelFromChromeBrowserState(
    ios::ChromeBrowserState* browser_state,
    TabModel* tab_model) {
  NSMutableSet<TabModel*>* tab_models =
      TabModelList::GetForBrowserState(browser_state, false)->tab_models();
  DCHECK([tab_models containsObject:tab_model]);
  [tab_models removeObject:tab_model];

  TabModelList* original_tab_model_list = TabModelList::GetForBrowserState(
      browser_state->GetOriginalChromeBrowserState(), false);
  for (auto& observer : original_tab_model_list->observers_)
    observer.TabModelUnregisteredFromBrowserState(tab_model, browser_state);
}

// static
NSArray<TabModel*>* TabModelList::GetTabModelsForChromeBrowserState(
    ios::ChromeBrowserState* browser_state) {
  TabModelList* tab_model_list =
      TabModelList::GetForBrowserState(browser_state, false);
  return tab_model_list ? [tab_model_list->tab_models() allObjects] : nil;
}

// static
TabModel* TabModelList::GetLastActiveTabModelForChromeBrowserState(
    ios::ChromeBrowserState* browser_state) {
  TabModelList* tab_model_list =
      TabModelList::GetForBrowserState(browser_state, false);
  if (!tab_model_list || [tab_model_list->tab_models() count] == 0u)
    return nil;

  // There is currently no way to mark a TabModel as active. Assert that there
  // is only one TabModel associated with |browser_state| until it is possible
  // to mark a TabModel as active.
  DCHECK_EQ([tab_model_list->tab_models() count], 1u);
  return [tab_model_list->tab_models() anyObject];
}

// static
bool TabModelList::IsOffTheRecordSessionActive() {
  std::vector<ios::ChromeBrowserState*> browser_states =
      GetApplicationContext()
          ->GetChromeBrowserStateManager()
          ->GetLoadedBrowserStates();

  for (ios::ChromeBrowserState* browser_state : browser_states) {
    DCHECK(!browser_state->IsOffTheRecord());
    if (!browser_state->HasOffTheRecordChromeBrowserState())
      continue;

    ios::ChromeBrowserState* otr_browser_state =
        browser_state->GetOffTheRecordChromeBrowserState();

    TabModelList* tab_model_list =
        TabModelList::GetForBrowserState(otr_browser_state, false);
    if (!tab_model_list)
      continue;

    for (TabModel* tab_model in tab_model_list->tab_models()) {
      if (![tab_model isEmpty])
        return true;
    }
  }

  return false;
}
