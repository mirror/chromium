// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#import "ios/chrome/browser/ui/settings/password_exporter.h"

#include "base/logging.h"
#include "base/mac/bind_objc_block.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
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

@interface PasswordExporter () {
  // Module containing the reauthentication mechanism used for exporting
  // passwords.
  __weak id<ReauthenticationProtocol> _weakReauthenticationModule;
  // Instance of the view controller initiating the export needed to
  // display various alerts.
  __weak id<PasswordExporterDelegate> _weakDelegate;
  // Name of the temporary passwords file. It can be used by the receiving app,
  // so it needs to be a localized string.
  NSString* _tempPasswordsFileName;
  // Task runner on which the serializing passwords and writing to file
  // operations should happen.
  scoped_refptr<base::SequencedTaskRunner> _sequencedTaskRunner;
}

@property(nonatomic, assign) BOOL reauthenticationSuccessful;
@property(nonatomic, assign) BOOL serializingFinished;
@property(atomic, assign) std::string serializedPasswords;

@end

@implementation PasswordExporter

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
    _weakReauthenticationModule = reauthenticationModule;
    _weakDelegate = delegate;
    _sequencedTaskRunner = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
  }
  return self;
}

- (void)startExportFlow:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords {
  _serializingFinished = NO;
  _reauthenticationSuccessful = NO;
  if ([_weakReauthenticationModule canAttemptReauth]) {
    [self serializePasswords:passwords];
    [self startReauthentication];
  } else {
    [_weakDelegate showSetPasscodeDialog];
  }
}

- (void)showExportErrorAlertWithLocalizedReason:(NSString*)errorReason {
  [_weakDelegate showExportErrorAlertWithLocalizedReason:errorReason];
}

#pragma mark -  Private methods

- (void)serializePasswords:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords {
  __weak PasswordExporter* weakSelf = self;
  void (^onPasswordsSerialized)(std::string) =
      ^(std::string serializedPasswords) {
        PasswordExporter* strongSelf = weakSelf;
        if (!strongSelf)
          return;
        strongSelf.serializedPasswords = serializedPasswords;
        strongSelf.serializingFinished = YES;
        [strongSelf writePasswordsToFile];
      };

  base::PostTaskAndReplyWithResult(
      _sequencedTaskRunner.get(), FROM_HERE,
      base::BindOnce(&password_manager::PasswordCSVWriter::SerializePasswords,
                     base::Passed(CopyOf(passwords))),
      base::OnceCallback<void(std::string)>(
          base::BindBlockArc(onPasswordsSerialized)));
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
                               handler:onReauthenticationFinished];
}

- (void)writePasswordsToFile {
  if (!_serializingFinished || !_reauthenticationSuccessful)
    return;
  // TODO: NSString*?
  std::string serializedPasswordsLocalString = self.serializedPasswords;

  WriteToURLStatus (^writeToFile)() = ^() {
    NSURL* passwordsTempFile =
        [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
            URLByAppendingPathComponent:_tempPasswordsFileName
                            isDirectory:NO];
    NSError* error = nil;
    NSString* serializedPasswords =
        base::SysUTF8ToNSString(serializedPasswordsLocalString);
    BOOL success = [serializedPasswords writeToURL:passwordsTempFile
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
        [self showExportErrorAlertWithLocalizedReason:
                  l10n_util::GetNSString(
                      IDS_IOS_EXPORT_PASSWORDS_OUT_OF_SPACE_ALERT_MESSAGE)];
        break;
      case UNKNOWN_ERROR:
        [self showExportErrorAlertWithLocalizedReason:
                  l10n_util::GetNSString(
                      IDS_IOS_EXPORT_PASSWORDS_UKNOWN_ERROR_ALERT_MESSAGE)];
        break;
      default:
        NOTREACHED();
    }
  };
  base::PostTaskAndReplyWithResult(
      _sequencedTaskRunner.get(), FROM_HERE,
      base::OnceCallback<WriteToURLStatus()>(base::BindBlockArc(writeToFile)),
      base::OnceCallback<void(WriteToURLStatus)>(
          base::BindBlockArc(onFileWritten)));
}

- (void)deleteTemporaryFile {
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSString* filePath = [NSTemporaryDirectory()
      stringByAppendingPathComponent:_tempPasswordsFileName];
  [fileManager removeItemAtPath:filePath error:nil];
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

@end
