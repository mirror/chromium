// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_DISCARDABLE_UNIT_H_

#include "chrome/browser/resource_coordinator/discardable_unit.h"

#include "base/macros.h"
#include "base/time/time.h"

class TabStripModel;

namespace content {
class RenderProcessHost;
class WebContents;
}  // namespace content

namespace resource_coordinator {

class TabManager;

class TabDiscardableUnit : public DiscardableUnit {
 public:
  TabDiscardableUnit(TabManager* tab_manager,
                     content::WebContents* web_contents,
                     TabStripModel* tab_strip_model);
  ~TabDiscardableUnit() override;

  void SetWebContents(content::WebContents* web_contents);

  void SetTabStripModel(TabStripModel* tab_strip_model);

  void SetActive(bool active);

  void SetRecentlyAudible(bool recently_audible);

  content::RenderProcessHost* GetRenderProcessHost() const;

  // DiscardableUnit:
  State GetState() const override;
  bool CanDiscard(SystemCondition system_condition) const override;
  void Discard(State state, SystemCondition system_condition) override;

 private:
  bool IsMediaTab() const;

  TabManager* const tab_manager_;
  content::WebContents* web_contents_;
  TabStripModel* tab_strip_model_;

  State state_ = State::ALIVE;
  int discard_count_ = 0;

  bool active_ = true;

  // The last time the tab switched from being active to inactive.
  base::TimeTicks last_inactive_time_;

  bool recently_audible_ = false;
  base::TimeTicks recently_audible_time_;

  bool auto_discardable_ = true;

  DISALLOW_COPY_AND_ASSIGN(TabDiscardableUnit);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_DISCARDABLE_UNIT_H_
