// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/password_exporter.h"

#include "base/logging.h"
#include "base/mac/bind_objc_block.h"
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

@interface PasswordFileWriter : NSObject<FileWriterProtocol>
@end

@implementation PasswordFileWriter

- (void)writeData:(NSString*)data
            toURL:(NSURL*)fileURL
          handler:(void (^)(WriteToURLStatus))handler {
  WriteToURLStatus (^writeToFile)() = ^() {
    base::AssertBlockingAllowed();
    NSError* error = nil;
    BOOL success = [data writeToURL:fileURL
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
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::OnceCallback<WriteToURLStatus()>(base::BindBlockArc(writeToFile)),
      base::OnceCallback<void(WriteToURLStatus)>(base::BindBlockArc(handler)));
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
  // Object that writes data to a file asyncronously and executes a handler
  // block when finished.
  id<FileWriterProtocol> _passwordFileWriter;
}

// Whether the reauthentication attempt was successful.
@property(nonatomic, assign) BOOL reauthenticationSuccessful;
// Whether the password serializing has finished.
@property(nonatomic, assign) BOOL serializingFinished;
// String containing serialized password forms.
@property(nonatomic, copy) NSString* serializedPasswords;
// Whether an export operation is ongoing. This is a readwrite property
// corresponding to the public readonly property.
@property(nonatomic, assign) BOOL isExporting;

@end

@implementation PasswordExporter

// Public synthesized properties
@synthesize isExporting = _isExporting;

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
    _passwordFileWriter = [[PasswordFileWriter alloc] init];
    _weakReauthenticationModule = reauthenticationModule;
    _weakDelegate = delegate;
    _isExporting = NO;
  }
  return self;
}

- (void)startExportFlow:
    (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)passwords {
  self.serializingFinished = NO;
  self.reauthenticationSuccessful = NO;
  if ([_weakReauthenticationModule canAttemptReauth]) {
    self.isExporting = YES;
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
    if (success) {
      strongSelf.reauthenticationSuccessful = success;
      [strongSelf writePasswordsToFile];
    } else {
      strongSelf.isExporting = NO;
    }
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

  NSURL* tempPasswordsFileURL =
      [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
          URLByAppendingPathComponent:_tempPasswordsFileName
                          isDirectory:NO];

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
        strongSelf.isExporting = NO;
        break;
      case UNKNOWN_ERROR:
        [strongSelf
            showExportErrorAlertWithLocalizedReason:
                l10n_util::GetNSString(
                    IDS_IOS_EXPORT_PASSWORDS_UNKNOWN_ERROR_ALERT_MESSAGE)];
        strongSelf.isExporting = NO;
        break;
      default:
        NOTREACHED();
    }
  };

  [_passwordFileWriter writeData:self.serializedPasswords
                           toURL:tempPasswordsFileURL
                         handler:onFileWritten];
}

- (void)deleteTemporaryFile:(NSURL*)passwordsTempFileURL {
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindBlockArc(^() {
        base::AssertBlockingAllowed();
        NSFileManager* fileManager = [NSFileManager defaultManager];
        [fileManager removeItemAtURL:passwordsTempFileURL error:nil];
      }));
}

- (void)showActivityView {
  NSURL* passwordsTempFileURL =
      [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
          URLByAppendingPathComponent:_tempPasswordsFileName
                          isDirectory:NO];

  __weak PasswordExporter* weakSelf = self;
  [_weakDelegate
      showActivityViewWithActivityItems:@[ passwordsTempFileURL ]
                      completionHandler:^(
                          NSString* activityType, BOOL completed,
                          NSArray* returnedItems, NSError* activityError) {
                        [weakSelf deleteTemporaryFile:passwordsTempFileURL];
                        // If |weakSelf| is nil, then |isExporting| will be set
                        // to NO when PasswordExporter will be initialized again
                        // in the future.
                        weakSelf.isExporting = NO;
                      }];
}

#pragma mark - ForTesting

- (void)setPasswordSerializerBridge:
    (id<PasswordSerializerBridge>)passwordSerializerBridge {
  _passwordSerializerBridge = passwordSerializerBridge;
}

- (void)setPasswordFileWriter:(id<FileWriterProtocol>)passwordFileWriter {
  _passwordFileWriter = passwordFileWriter;
}

@end
