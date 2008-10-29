// Copyright (c) 2008 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_BLOCKED_POPUP_CONTAINER_H
#define CHROME_BROWSER_VIEWS_BLOCKED_POPUP_CONTAINER_H

#include "base/gfx/rect.h"
#include "chrome/browser/constrained_window.h"
#include "chrome/common/pref_member.h"
#include "chrome/views/container_win.h"

#include <vector>

class BlockedPopupContainerView;
class Profile;
class TabContents;
class TextButton;

///////////////////////////////////////////////////////////////////////////////
//
// BlockedPopupContainer
//
//  This class takes ownership of TabContents that are unrequested popup
//  windows and presents an interface to the user for launching them. (Or never
//  showing them again)
//
class BlockedPopupContainer : public ConstrainedWindow,
                              public views::ContainerWin {
 public:
  virtual ~BlockedPopupContainer();

  // Create a BlockedPopupContainer, anchoring the container to the lower right
  // corner.
  static BlockedPopupContainer* Create(
      TabContents* owner, Profile* profile, const gfx::Point& initial_anchor);

  // Toggles the preference to display this notification.
  void ToggleBlockedPopupNotification();

  // Gets the current state of the show blocked popup notification preference.
  bool GetShowBlockedPopupNotification();

  // Adds a Tabbed contents to this container
  void AddTabContents(TabContents* blocked_contents, const gfx::Rect& bounds);

  // Creates a window from blocked popup |index|.
  void LaunchPopupIndex(int index);

  // Return the number of blocked popups
  int GetTabContentsCount() const;

  // Returns the string to display to the user in the menu for item |index|.
  std::wstring GetDisplayStringForItem(int index);

  // Deletes all popups and hides the interface parts.
  void CloseAllPopups();

  // Override from ConstrainedWindow:
  virtual void ActivateConstrainedWindow();
  virtual void CloseConstrainedWindow();
  virtual void RepositionConstrainedWindowTo(const gfx::Point& anchor_point);
  virtual void WasHidden();
  virtual void DidBecomeSelected();
  virtual std::wstring GetWindowTitle() const;
  virtual void UpdateWindowTitle();
  virtual const gfx::Rect& GetCurrentBounds() const;

 protected:
  // A bunch of windows messages I don't know about.

 private:
  // 
  BlockedPopupContainer(TabContents* owner, Profile* profile);

  // Initialize our Views and positions us to the lower right corner of the
  // browser window.
  void Init(const gfx::Point& initial_anchor);

  // The TabContents that owns and constrains this BlockedPopupContainer.
  TabContents* owner_;

  // TabContents.
  std::vector<std::pair<TabContents*, gfx::Rect> > blocked_popups_;

  // Our associated view object.
  BlockedPopupContainerView* container_view_;

  // Link to the show blocked popup preference. Used to both determine whether
  // we should show ourself to the user and whether we 
  BooleanPrefMember disable_popup_blocked_notification_pref_;

  gfx::Rect bounds_;
};

#endif
