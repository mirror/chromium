// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_SOURCE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_SOURCE_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/render_process_host_observer.h"

namespace content {
class RenderProcessHost;
class WebContents;
}  // namespace content

namespace resource_coordinator {

class TabDiscardableUnit;
class TabManager;

class TabDiscardableUnitSource : public TabStripModelObserver,
                                 public chrome::BrowserListObserver {
 public:
  TabDiscardableUnitSource(TabManager* tab_manager);
  ~TabDiscardableUnitSource() override;

 private:
  void NotifyFocusedTab();

  // TabStripModelObserver:
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;

  // chrome::BrowserListObserver:
  void OnBrowserSetLastActive(Browser* browser) override;

  BrowserTabStripTracker browser_tab_strip_tracker_;

  TabManager* tab_manager_;

  base::flat_map<content::WebContents*, std::unique_ptr<TabDiscardableUnit>>
      tabs_;

  DISALLOW_COPY_AND_ASSIGN(TabDiscardableUnitSource);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_SOURCE_H_
