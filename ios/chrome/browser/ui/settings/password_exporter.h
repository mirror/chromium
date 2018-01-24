// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <vector>

namespace autofill {
struct PasswordForm;
}  // namespace autofill

@protocol ReauthenticationProtocol;

@protocol PasswordSerializerBridge<NSObject>

// Posts task to serialize passwords and calls |serializedPasswordsHandler|
// when serialization is finished.
- (void)serializePasswords:
            (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)
                passwords
                   handler:(void (^)(std::string))serializedPasswordsHandler;

@end

@protocol PasswordExporterDelegate<NSObject>

// Displays a dialog informing the user that they must set up a passcode
// in order to export passwords.
- (void)showSetPasscodeDialog;

// Displays an alert detailing an error that has occured during export.
- (void)showExportErrorAlertWithLocalizedReason:(NSString*)errorReason;

// Displays an activity view that allows the user to pick an app to process
// the exported passwords file.
- (void)showActivityViewWithActivityItems:(NSArray*)activityItems
                        completionHandler:
                            (void (^)(NSString* activityType,
                                      BOOL completed,
                                      NSArray* returnedItems,
                                      NSError* activityError))completionHandler;

@end

/** Class handling all the operations necessary to export passwords.*/
@interface PasswordExporter : NSObject

// The designated initializer. |reauthenticationModule| and |delegate| must
// not be nil.
- (instancetype)initWithReauthenticationModule:
                    (id<ReauthenticationProtocol>)reuthenticationModule
                                      delegate:
                                          (id<PasswordExporterDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Method to be called in order to start the export flow. This initiates
// the reauthentication procedure and asks for password serialization.
- (void)startExportFlow:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_
