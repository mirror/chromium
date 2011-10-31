// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_TAB_FIRST_RENDER_WATCHER_H_
#define CHROME_BROWSER_CHROMEOS_TAB_FIRST_RENDER_WATCHER_H_
#pragma once

#include "base/compiler_specific.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class RenderViewHost;
class TabContents;

namespace chromeos {

// This class watches given TabContent's loading and rendering state change.
// TODO(xiyuan): Move this to a proper place and share with HTMLDialogView.
class TabFirstRenderWatcher : public content::NotificationObserver {
 public:
  class Delegate {
   public:
    virtual void OnRenderHostCreated(RenderViewHost* host) = 0;
    virtual void OnTabMainFrameLoaded() = 0;
    virtual void OnTabMainFrameFirstRender() = 0;
  };

  TabFirstRenderWatcher(TabContents* tab, Delegate* delegate);

 private:
  // Overridden from content::NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  enum State {
    NONE,
    LOADED,        // Renderer loaded the page.
    FIRST_PAINT,   // 1st paint event after the page is loaded.
  };
  State state_;

  // TabContents that this class watches.
  TabContents* tab_contents_;

  // Delegate to notify.
  Delegate* delegate_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(TabFirstRenderWatcher);
};

}   // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_TAB_FIRST_RENDER_WATCHER_H_
