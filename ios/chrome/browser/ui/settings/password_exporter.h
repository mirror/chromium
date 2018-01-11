// Copyright 2017 The Chromium Authors. All rights reserved.
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

@protocol PasswordExporterDelegate<NSObject>

- (void)showSetPasscodeDialog;

- (void)showExportErrorAlertWithLocalizedReason:(NSString*)errorReason;

- (void)showActivityViewWithActivityItems:(NSArray*)activityItems
                        completionHandler:
                            (void (^)(NSString* activityType,
                                      BOOL completed,
                                      NSArray* returnedItems,
                                      NSError* activityError))completionHandler;

@end

/** Class handling all the operations necessary to export passwords.*/
@interface PasswordExporter : NSObject

- (instancetype)initWithReauthenticationModule:
                    (id<ReauthenticationProtocol>)reuthenticationModule
                                      delegate:
                                          (id<PasswordExporterDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

- (void)startExportFlow:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_
