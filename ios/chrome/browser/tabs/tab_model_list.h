// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_MODEL_LIST_H_
#define IOS_CHROME_BROWSER_TABS_TAB_MODEL_LIST_H_

#import <Foundation/Foundation.h>

#include "base/observer_list.h"
#include "base/supports_user_data.h"

@class TabModel;

class TabModelListObserver;

namespace ios {
class ChromeBrowserState;
}

// Wrapper around a NSMutableArray<TabModel> allowing to bind it to a
// base::SupportsUserData. Any base::SupportsUserData that owns this
// wrapper will owns the reference to the TabModels.
class TabModelList : public base::SupportsUserData::Data {
 public:
  TabModelList();
  ~TabModelList() override;

  static TabModelList* GetForBrowserState(
      ios::ChromeBrowserState* browser_state,
      bool create);

  // Adds |observer| to the list of observers.
  static void AddObserver(TabModelListObserver* observer);

  // Removes |observer| from the list of observers.
  static void RemoveObserver(TabModelListObserver* observer);

  // Registers |tab_model| as associated to |browser_state|. The object will
  // be retained until |UnregisterTabModelFromChromeBrowserState| is called.
  // It is an error if |tab_model is already registered as associated to
  // |browser_state|.
  static void RegisterTabModelWithChromeBrowserState(
      ios::ChromeBrowserState* browser_state,
      TabModel* tab_model);

  // Unregisters the association between |tab_model| and |browser_state|.
  // It is an error if no such association exists.
  static void UnregisterTabModelFromChromeBrowserState(
      ios::ChromeBrowserState* browser_state,
      TabModel* tab_model);

  // Returns the list of all TabModels associated with |browser_state|.
  static NSArray<TabModel*>* GetTabModelsForChromeBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Returns the last active TabModel associated with |browser_state|.
  static TabModel* GetLastActiveTabModelForChromeBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Returns true if a incognito session is currently active (i.e. at least
  // one incognito tab is open).
  static bool IsOffTheRecordSessionActive();

 private:
  NSMutableSet<TabModel*>* tab_models() const { return tab_models_; }

  NSMutableSet<TabModel*>* tab_models_;
  base::ObserverList<TabModelListObserver, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(TabModelList);
};

#endif  // IOS_CHROME_BROWSER_TABS_TAB_MODEL_LIST_H_
