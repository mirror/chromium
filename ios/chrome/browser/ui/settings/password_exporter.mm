// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/password_exporter.h"

#include "base/logging.h"
#include "base/mac/bind_objc_block.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/export/password_csv_writer.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/settings/reauthentication_module.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

enum WriteToURLStatus {
  SUCCESS,
  OUT_OF_DISK_SPACE_ERROR,
  UNKNOWN_ERROR,
};

std::vector<std::unique_ptr<autofill::PasswordForm>> CopyOf(
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& passwordList) {
  std::vector<std::unique_ptr<autofill::PasswordForm>> passwordListCopy;
  for (const auto& form : passwordList) {
    passwordListCopy.push_back(std::make_unique<autofill::PasswordForm>(*form));
  }
  return passwordListCopy;
}

}  // namespace

@interface PasswordSerializerBridge : NSObject<PasswordSerializerBridge>
@end

@implementation PasswordSerializerBridge

- (void)serializePasswords:
            (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)
                passwords
                   handler:(void (^)(std::string))serializedPasswordsHandler {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(&password_manager::PasswordCSVWriter::SerializePasswords,
                     base::Passed(CopyOf(passwords))),
      base::OnceCallback<void(std::string)>(
          base::BindBlockArc(serializedPasswordsHandler)));
}

@end

@interface PasswordExporter () {
  // Module containing the reauthentication mechanism used for exporting
  // passwords.
  __weak id<ReauthenticationProtocol> _weakReauthenticationModule;
  // Instance of the view controller initiating the export. Used
  // for displaying alerts.
  __weak id<PasswordExporterDelegate> _weakDelegate;
  // Name of the temporary passwords file. It can be used by the receiving app,
  // so it needs to be a localized string.
  NSString* _tempPasswordsFileName;
  // Bridge object that triggers password serialization and executes a
  // handler on the serialized passwords.
  id<PasswordSerializerBridge> _passwordSerializerBridge;
}

// Whether the reauthentication attempt was successful.
@property(nonatomic, assign) BOOL reauthenticationSuccessful;
// Whether the password serializing has finished.
@property(nonatomic, assign) BOOL serializingFinished;
// String containing serialized password forms.
@property(nonatomic, copy) NSString* serializedPasswords;

@end

@implementation PasswordExporter

// Private synthesized properties
@synthesize reauthenticationSuccessful = _reauthenticationSuccessful;
@synthesize serializingFinished = _serializingFinished;
@synthesize serializedPasswords = _serializedPasswords;

- (instancetype)initWithReauthenticationModule:
                    (id<ReauthenticationProtocol>)reauthenticationModule
                                      delegate:(id<PasswordExporterDelegate>)
                                                   delegate {
  DCHECK(delegate);
  DCHECK(reauthenticationModule);
  self = [super init];
  if (self) {
    _tempPasswordsFileName =
        [l10n_util::GetNSString(IDS_PASSWORD_MANAGER_DEFAULT_EXPORT_FILENAME)
            stringByAppendingString:@".csv"];
    _passwordSerializerBridge = [[PasswordSerializerBridge alloc] init];
    _weakReauthenticationModule = reauthenticationModule;
    _weakDelegate = delegate;
  }
  return self;
}

- (void)startExportFlow:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords {
  self.serializingFinished = NO;
  self.reauthenticationSuccessful = NO;
  if ([_weakReauthenticationModule canAttemptReauth]) {
    [self serializePasswords:passwords];
    [self startReauthentication];
  } else {
    [_weakDelegate showSetPasscodeDialog];
  }
}

#pragma mark -  Private methods

- (void)showExportErrorAlertWithLocalizedReason:(NSString*)errorReason {
  [_weakDelegate showExportErrorAlertWithLocalizedReason:errorReason];
}

- (void)serializePasswords:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords {
  __weak PasswordExporter* weakSelf = self;
  void (^onPasswordsSerialized)(std::string) =
      ^(std::string serializedPasswords) {
        PasswordExporter* strongSelf = weakSelf;
        if (!strongSelf)
          return;
        strongSelf.serializedPasswords =
            base::SysUTF8ToNSString(serializedPasswords);
        strongSelf.serializingFinished = YES;
        [strongSelf writePasswordsToFile];
      };
  [_passwordSerializerBridge serializePasswords:passwords
                                        handler:onPasswordsSerialized];
}

- (void)startReauthentication {
  __weak PasswordExporter* weakSelf = self;

  void (^onReauthenticationFinished)(BOOL) = ^(BOOL success) {
    PasswordExporter* strongSelf = weakSelf;
    if (!strongSelf)
      return;
    strongSelf.reauthenticationSuccessful = success;
    [strongSelf writePasswordsToFile];
  };

  [_weakReauthenticationModule
      attemptReauthWithLocalizedReason:l10n_util::GetNSString(
                                           IDS_IOS_EXPORT_PASSWORDS)
                  canReusePreviousAuth:NO
                               handler:onReauthenticationFinished];
}

- (void)writePasswordsToFile {
  if (!_serializingFinished || !_reauthenticationSuccessful)
    return;

  NSString* serializedPasswords = self.serializedPasswords;
  NSString* tempPasswordsFileName = _tempPasswordsFileName;

  WriteToURLStatus (^writeToFile)() = ^() {
    base::AssertBlockingAllowed();
    NSURL* tempPasswordsFile =
        [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
            URLByAppendingPathComponent:tempPasswordsFileName
                            isDirectory:NO];
    NSError* error = nil;
    BOOL success = [serializedPasswords writeToURL:tempPasswordsFile
                                        atomically:YES
                                          encoding:NSUTF8StringEncoding
                                             error:&error];
    if (!success) {
      if (error.code == NSFileWriteOutOfSpaceError) {
        return OUT_OF_DISK_SPACE_ERROR;
      } else {
        return UNKNOWN_ERROR;
      }
    }
    return SUCCESS;
  };

  __weak PasswordExporter* weakSelf = self;
  void (^onFileWritten)(WriteToURLStatus) = ^(WriteToURLStatus status) {
    PasswordExporter* strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    switch (status) {
      case SUCCESS:
        [strongSelf showActivityView];
        break;
      case OUT_OF_DISK_SPACE_ERROR:
        [strongSelf
            showExportErrorAlertWithLocalizedReason:
                l10n_util::GetNSString(
                    IDS_IOS_EXPORT_PASSWORDS_OUT_OF_SPACE_ALERT_MESSAGE)];
        break;
      case UNKNOWN_ERROR:
        [strongSelf
            showExportErrorAlertWithLocalizedReason:
                l10n_util::GetNSString(
                    IDS_IOS_EXPORT_PASSWORDS_UKNOWN_ERROR_ALERT_MESSAGE)];
        break;
      default:
        NOTREACHED();
    }
  };

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::OnceCallback<WriteToURLStatus()>(base::BindBlockArc(writeToFile)),
      base::OnceCallback<void(WriteToURLStatus)>(
          base::BindBlockArc(onFileWritten)));
}

- (void)deleteTemporaryFile {
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindBlockArc(^() {
        base::AssertBlockingAllowed();
        NSFileManager* fileManager = [NSFileManager defaultManager];
        NSURL* fileURL =
            [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
                URLByAppendingPathComponent:_tempPasswordsFileName
                                isDirectory:NO];
        [fileManager removeItemAtURL:fileURL error:nil];
      }));
}

- (void)showActivityView {
  NSURL* passwordsTempFile =
      [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
          URLByAppendingPathComponent:_tempPasswordsFileName
                          isDirectory:NO];

  __weak PasswordExporter* weakSelf = self;
  [_weakDelegate
      showActivityViewWithActivityItems:@[ passwordsTempFile ]
                      completionHandler:^(
                          NSString* activityType, BOOL completed,
                          NSArray* returnedItems, NSError* activityError) {
                        [weakSelf deleteTemporaryFile];
                      }];
}

#pragma mark - ForTesting
- (void)setPasswordSerializerBridge:
    (id<PasswordSerializerBridge>)passwordSerializerBridge {
  _passwordSerializerBridge = passwordSerializerBridge;
}

@end
