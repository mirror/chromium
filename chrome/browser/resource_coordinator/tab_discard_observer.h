// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARD_OBSERVER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARD_OBSERVER_H_

namespace content {
class WebContents;
}

namespace resource_coordinator {

// Interface to be notified of events related to tab discarding.
class TabDiscardObserver {
 public:
  // Invoked when |contents| is discarded or reloaded after a discard.
  // |is_discarded| indicates the new state of |contents|.
  virtual void OnDiscardedStateChange(content::WebContents* contents,
                                      bool is_discarded);

  // Invoked when the auto-discardable state of |contents| changes.
  // |is_auto_discardable| is the new auto-discardable state of |contents|.
  virtual void OnAutoDiscardableStateChange(content::WebContents* contents,
                                            bool is_auto_discardable);

 protected:
  virtual ~TabDiscardObserver();
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARD_OBSERVER_H_
