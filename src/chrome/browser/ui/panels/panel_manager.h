// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PANELS_PANEL_MANAGER_H_
#define CHROME_BROWSER_UI_PANELS_PANEL_MANAGER_H_
#pragma once

#include <deque>
#include <vector>
#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/ui/panels/panel.h"
#include "ui/gfx/rect.h"

class Browser;
class Panel;

// This class manages a set of panels.
class PanelManager {
 public:
  // Returns a single instance.
  static PanelManager* GetInstance();

  ~PanelManager();

  // Called when the display is changed, i.e. work area is updated.
  void OnDisplayChanged();

  // Creates a panel and returns it. The panel might be queued for display
  // later.
  Panel* CreatePanel(Browser* browser);

  // Removes the given panel. Both active and pending panel lists are checked.
  // If an active panel is removed, pending panels could be displayed if space
  // allows.
  void Remove(Panel* panel);

  // Removes all active panels. Pending panels will be processed for display.
  void RemoveAllActive();

  // Drags the given active panel.
  void StartDragging(Panel* panel);
  void Drag(int delta_x);
  void EndDragging(bool cancelled);

  // Should we bring up the titlebar, given the current mouse point?
  bool ShouldBringUpTitleBarForAllMinimizedPanels(int mouse_x,
                                                  int mouse_y) const;

  // Brings up or down the title-bar for all minimized panels.
  void BringUpOrDownTitleBarForAllMinimizedPanels(bool bring_up);

  // Returns the number of active panels.
  int active_count() const { return active_panels_.size(); }

 private:
  typedef std::vector<Panel*> ActivePanels;
  typedef std::deque<Panel*> PendingPanels;

  PanelManager();

  // Handles all the panels that're delayed to be removed.
  void DelayedRemove();

  // Does the remove. Called from Remove and DelayedRemove.
  void DoRemove(Panel* panel);

  // Rearranges the positions of the panels starting from the given iterator.
  // This is called when the display space has been changed, i.e. working
  // area being changed or a panel being closed.
  void Rearrange(ActivePanels::iterator iter_to_start);

  // Checks the pending panels to see if we show them when we have more space.
  void ProcessPending();

  // Computes the bounds for next panel.
  // |allow_size_change| is used to indicate if the panel size can be changed to
  // fall within the size constraint, e.g., when the panel is created.
  // Returns true if computed bounds are within the displayable area.
  bool ComputeBoundsForNextPanel(gfx::Rect* bounds, bool allow_size_change);

  // Help functions to drag the given panel.
  void DragLeft();
  void DragRight();

  // Stores the active panels.
  ActivePanels active_panels_;

  // Stores the panels that are pending to show.
  PendingPanels pending_panels_;

  // Stores the panels that are pending to remove. We want to delay the removal
  // when we're in the process of the dragging.
  std::vector<Panel*> panels_pending_to_remove_;

  // Current work area used in computing the panel bounds.
  gfx::Rect work_area_;

  // Used in computing the bounds of the next panel.
  int max_width_;
  int max_height_;
  int min_x_;
  int current_x_;
  int bottom_edge_y_;

  // Panel to drag.
  size_t dragging_panel_index_;

  // Original x coordinate of the panel to drag. This is used to get back to
  // the original position when we cancel the dragging.
  int dragging_panel_original_x_;

  // Bounds of the panel to drag. It is first set to the original bounds when
  // the dragging happens. Then it is updated to the position that will be set
  // to when the dragging ends.
  gfx::Rect dragging_panel_bounds_;

  DISALLOW_COPY_AND_ASSIGN(PanelManager);
};

#endif  // CHROME_BROWSER_UI_PANELS_PANEL_MANAGER_H_
