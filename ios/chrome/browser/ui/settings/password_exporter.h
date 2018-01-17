// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_

#import <Foundation/Foundation.h>

@protocol ReauthenticationProtocol;

@protocol PasswordExporterDelegate<NSObject>

- (void)showSetPasscodeDialog;

@end

/** Class handling all the operations necessary to export passwords.*/
@interface PasswordExporter : NSObject

- (instancetype)initWithReauthenticationModule:
                    (id<ReauthenticationProtocol>)reuthenticationModule
                                      delegate:
                                          (id<PasswordExporterDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

- (void)startExportFlow;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_PASSWORD_EXPORTER_H_
