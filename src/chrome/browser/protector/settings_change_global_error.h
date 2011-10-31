// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROTECTOR_SETTINGS_CHANGE_GLOBAL_ERROR_H_
#define CHROME_BROWSER_PROTECTOR_SETTINGS_CHANGE_GLOBAL_ERROR_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/protector/setting_change.h"
#include "chrome/browser/ui/global_error.h"

class Browser;
class Profile;

namespace protector {

class SettingsChangeGlobalErrorDelegate;

// Global error about unwanted settings changes.
class SettingsChangeGlobalError : public GlobalError {
 public:
  // Creates new global error about settings changes |changes|. Takes
  // ownership over |changes| contents.
  // Uses |delegate| to notify about user decision.
  SettingsChangeGlobalError(
      const SettingChangeVector& changes,
      SettingsChangeGlobalErrorDelegate* delegate);
  virtual ~SettingsChangeGlobalError();

  // Displays a global error bubble for the given browser profile.
  // Can be called from any thread.
  void ShowForProfile(Profile* profile);

  // Browser that the bubble has been last time shown for.
  Browser* browser() const { return browser_; }

  SettingChangeVector* mutable_changes() { return &changes_; }

  // GlobalError implementation.
  virtual bool HasBadge() OVERRIDE;
  virtual bool HasMenuItem() OVERRIDE;
  virtual int MenuItemCommandID() OVERRIDE;
  virtual string16 MenuItemLabel() OVERRIDE;
  virtual void ExecuteMenuItem(Browser* browser) OVERRIDE;
  virtual bool HasBubbleView() OVERRIDE;
  virtual string16 GetBubbleViewTitle() OVERRIDE;
  virtual string16 GetBubbleViewMessage() OVERRIDE;
  virtual string16 GetBubbleViewAcceptButtonLabel() OVERRIDE;
  virtual string16 GetBubbleViewCancelButtonLabel() OVERRIDE;
  virtual bool IsAcceptButtonDefault() OVERRIDE;
  virtual void BubbleViewDidClose() OVERRIDE;
  virtual void BubbleViewAcceptButtonPressed() OVERRIDE;
  virtual void BubbleViewCancelButtonPressed() OVERRIDE;

 private:
  // Helper called on the UI thread to add this global error to the default
  // profile (stored in |profile_|).
  void AddToProfile(Profile* profile);

  // Displays global error bubble. Must be called on the UI thread.
  void Show();

  // Removes global error from its profile and deletes |this| later.
  void RemoveFromProfile();

  // List of changes to show.
  SettingChangeVector changes_;

  // Delegate to notify about user actions.
  SettingsChangeGlobalErrorDelegate* delegate_;

  // Profile that we have been added to.
  Profile* profile_;

  // Browser that we have been shown for.
  Browser* browser_;

  // True if user has dismissed the bubble by clicking on one of the buttons.
  bool closed_by_button_;

  base::WeakPtrFactory<SettingsChangeGlobalError> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SettingsChangeGlobalError);
};

}  // namespace protector

#endif  // CHROME_BROWSER_PROTECTOR_SETTINGS_CHANGE_GLOBAL_ERROR_H_
