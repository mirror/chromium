// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_BROWSER_STATE_DATA_REMOVER_H_
#define IOS_CHROME_BROWSER_SIGNIN_BROWSER_STATE_DATA_REMOVER_H_

#include "base/ios/block_types.h"
#include "base/macros.h"
#include "ios/chrome/browser/browsing_data/ios_chrome_browsing_data_remover.h"

@class BrowsingDataRemovalController;

namespace ios {
class ChromeBrowserState;
}

namespace reading_list {
class ReadingListRemoverHelper;
}

// Helper that wipes all the data in the given browser state. This deletes all
// browsing data and all the bookmarks.
class BrowserStateDataRemover {
 public:
  ~BrowserStateDataRemover();

  // Removes all bookmarks, clears all browsing data, last signed-in username
  // and then runs |completion|. The user must be signed out when this method
  // is called.
  static void ClearData(ios::ChromeBrowserState* browser_state,
                        BrowsingDataRemovalController* controller,
                        ProceduralBlock completion);

 private:
  explicit BrowserStateDataRemover(ios::ChromeBrowserState* browser_state,
                                   ProceduralBlock completion);

  // Wipes all the data in the browser state and invokes |completion_block_|
  // when done and then delete itself. This method can only be called once.
  void RemoveBrowserStateData(BrowsingDataRemovalController* controller);

  void NotifyWithDetails(
      const IOSChromeBrowsingDataRemover::NotificationDetails& details);
  void ReadingListCleaned(
      const IOSChromeBrowsingDataRemover::NotificationDetails& details,
      bool reading_list_cleaned);

  ios::ChromeBrowserState* browser_state_;
  ProceduralBlock completion_block_;
  IOSChromeBrowsingDataRemover::CallbackSubscription callback_subscription_;
  std::unique_ptr<reading_list::ReadingListRemoverHelper>
      reading_list_remover_helper_;

  DISALLOW_COPY_AND_ASSIGN(BrowserStateDataRemover);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_BROWSER_STATE_DATA_REMOVER_H_
