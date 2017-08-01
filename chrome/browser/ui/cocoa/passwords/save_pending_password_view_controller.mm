// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/passwords/save_pending_password_view_controller.h"

#import "chrome/browser/ui/cocoa/passwords/password_item_views.h"
#import "chrome/browser/ui/cocoa/passwords/passwords_bubble_utils.h"
#import "chrome/browser/ui/cocoa/passwords/passwords_list_view_controller.h"
#include "chrome/browser/ui/passwords/manage_passwords_bubble_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "ui/base/l10n/l10n_util.h"

@interface SavePendingPasswordViewController ()<NSTextFieldDelegate>
- (void)onSaveClicked:(id)sender;
- (void)onNeverForThisSiteClicked:(id)sender;
- (void)onEditClicked:(id)sender;
- (void)refreshRow;
@end

@implementation SavePendingPasswordViewController

- (NSButton*)defaultButton {
  return saveButton_;
}

- (void)onSaveClicked:(id)sender {
  ManagePasswordsBubbleModel* model = self.model;
  if (model) {
    model->OnSaveClicked();
    if (model->ReplaceToShowPromotionIfNeeded()) {
      [self.delegate refreshBubble];
      return;
    }
  }
  [self.delegate viewShouldDismiss];
}

- (void)onNeverForThisSiteClicked:(id)sender {
  ManagePasswordsBubbleModel* model = self.model;
  if (model)
    model->OnNeverForThisSiteClicked();
  [self.delegate viewShouldDismiss];
}

- (void)onEditClicked:(id)sender {
  editMode_ = TRUE;
  [editButton_ setEnabled:FALSE];
  [self refreshRow];
}

// Handler for editable username field.
- (BOOL)control:(NSControl*)control
               textView:(NSTextView*)textView
    doCommandBySelector:(SEL)commandSelector {
  if (commandSelector == @selector(insertTab:) ||
      commandSelector == @selector(insertNewline:) ||
      commandSelector == @selector(cancelOperation:)) {
    // TODO(crbug.com/734965): Update the username credential.
    editMode_ = FALSE;
    [editButton_ setEnabled:TRUE];
    [self refreshRow];
    [[passwordItem_ window] setDefaultButtonCell:[saveButton_ cell]];
    return TRUE;
  }
  return FALSE;
}

- (NSView*)createPasswordView {
  if (!base::FeatureList::IsEnabled(
          password_manager::features::kEnableUsernameCorrection) &&
      self.model->pending_password().username_value.empty())
    return nil;
  PendingPasswordItemView* item = [[PendingPasswordItemView alloc]
      initWithForm:self.model->pending_password()
          editMode:editMode_];
  [item layoutWithFirstColumn:[item firstColumnWidth]
                 secondColumn:[item secondColumnWidth]];
  passwordItem_.reset([[PendingPasswordItemView alloc]
      initWithForm:self.model->pending_password()
          editMode:editMode_]);
  [passwordItem_ setSubviews:@[ item ]];
  [passwordItem_ setFrameSize:[item frame].size];
  return passwordItem_;
}

- (void)refreshRow {
  PendingPasswordItemView* item = [[PendingPasswordItemView alloc]
      initWithForm:self.model->pending_password()
          editMode:editMode_];
  [item layoutWithFirstColumn:[item firstColumnWidth]
                 secondColumn:[item secondColumnWidth]];
  [passwordItem_ setSubviews:@[ item ]];
  if (editMode_) {
    [[passwordItem_ window] makeFirstResponder:[item usernameField]];
    [[item usernameField] setDelegate:self];
  }
}

- (NSArray*)createButtonsAndAddThemToView:(NSView*)view {
  // Save button.
  saveButton_.reset(
      [[self addButton:l10n_util::GetNSString(IDS_PASSWORD_MANAGER_SAVE_BUTTON)
                toView:view
                target:self
                action:@selector(onSaveClicked:)] retain]);

  // Never button.
  NSString* neverButtonText =
      l10n_util::GetNSString(IDS_PASSWORD_MANAGER_BUBBLE_BLACKLIST_BUTTON);
  neverButton_.reset(
      [[self addButton:neverButtonText
                toView:view
                target:self
                action:@selector(onNeverForThisSiteClicked:)] retain]);
  if (base::FeatureList::IsEnabled(
          password_manager::features::kEnableUsernameCorrection)) {
    // Edit button.
    editButton_.reset([[self
        addButton:l10n_util::GetNSString(IDS_PASSWORD_MANAGER_EDIT_BUTTON)
           toView:view
           target:self
           action:@selector(onEditClicked:)] retain]);
    return @[ saveButton_, neverButton_, editButton_ ];
  }
  return @[ saveButton_, neverButton_ ];
}

@end

@implementation SavePendingPasswordViewController (Testing)

- (NSButton*)editButton {
  return editButton_.get();
}

- (NSButton*)saveButton {
  return saveButton_.get();
}

- (NSButton*)neverButton {
  return neverButton_.get();
}

@end
