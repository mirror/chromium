// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_DIALOG_TEXT_FIELD_CONFIGURATION_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_DIALOG_TEXT_FIELD_CONFIGURATION_H_

#import <Foundation/Foundation.h>

@class DialogConfigurationIdentifier;

// An object encapsulating the data necessary to set up a dialog's text field.
@interface DialogTextFieldConfiguration : NSObject

// Factory method for item creation.
+ (instancetype)configWithDefaultText:(NSString*)defaultText
                      placeholderText:(NSString*)placeholderText
                               secure:(BOOL)secure;

// DialogTextFieldConfigurations should be created through the factory method.
- (instancetype)init NS_UNAVAILABLE;

// The default text to display in the text field, if any.
@property(nonatomic, readonly, copy) NSString* defaultText;

// The placehodler text to display in the text field, if any.
@property(nonatomic, readonly, copy) NSString* placeholderText;

// Whether the text field should be secure (e.g. for password).
@property(nonatomic, readonly, getter=isSecure) BOOL secure;

// Unique identifier for this DialogTextFieldConfiguration.
@property(nonatomic, readonly, strong)
    DialogConfigurationIdentifier* identifier;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_DIALOGS_DIALOG_TEXT_FIELD_CONFIGURATION_H_
