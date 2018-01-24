// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/password_exporter_for_testing.h"

#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/autofill/core/common/password_form.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/test/app/password_test_util.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FakePasswordSerialzerBridge : NSObject<PasswordSerializerBridge>

// Allows for on demand execution of the block that handles the serialized
// passwords.
- (void)executeHandler;

@end

@implementation FakePasswordSerialzerBridge {
  // Handler processing the serialized passwords.
  void (^_serializedPasswordsHandler)(std::string);
}

- (void)serializePasswords:
            (const std::vector<std::unique_ptr<autofill::PasswordForm>>&)
                passwords
                   handler:(void (^)(std::string))serializedPasswordsHandler {
  _serializedPasswordsHandler = serializedPasswordsHandler;
}

- (void)executeHandler {
  _serializedPasswordsHandler("serialized passwords");
}
@end

namespace {
class PasswordExporterTest : public PlatformTest {
 public:
  PasswordExporterTest() = default;

 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    mock_reauthentication_module_ = [[MockReauthenticationModule alloc] init];
    password_exporter_delegate_ =
        OCMProtocolMock(@protocol(PasswordExporterDelegate));
    password_exporter_ = [[PasswordExporter alloc]
        initWithReauthenticationModule:mock_reauthentication_module_
                              delegate:password_exporter_delegate_];
  }

  std::vector<std::unique_ptr<autofill::PasswordForm>> CreatePasswordList() {
    auto password_form = std::make_unique<autofill::PasswordForm>();
    password_form->origin = GURL("http://accounts.google.com/a/LoginAuth");
    password_form->username_value = base::ASCIIToUTF16("test@testmail.com");
    password_form->password_value = base::ASCIIToUTF16("test1");

    std::vector<std::unique_ptr<autofill::PasswordForm>> password_forms;
    password_forms.push_back(std::move(password_form));
    return password_forms;
  }

  void TearDown() override {
    NSURL* passwordsFileURL = GetPasswordsFileURL();
    if ([[NSFileManager defaultManager]
            fileExistsAtPath:[passwordsFileURL path]]) {
      [[NSFileManager defaultManager] removeItemAtURL:passwordsFileURL
                                                error:nil];
    }
    PlatformTest::TearDown();
  }

  NSURL* GetPasswordsFileURL() {
    NSString* passwordsFileName =
        [l10n_util::GetNSString(IDS_PASSWORD_MANAGER_DEFAULT_EXPORT_FILENAME)
            stringByAppendingString:@".csv"];
    return [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
        URLByAppendingPathComponent:passwordsFileName
                        isDirectory:NO];
  }

  BOOL PasswordFileExists() {
    return [[NSFileManager defaultManager]
        fileExistsAtPath:[GetPasswordsFileURL() path]];
  }

  id password_exporter_delegate_;
  PasswordExporter* password_exporter_;
  MockReauthenticationModule* mock_reauthentication_module_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(PasswordExporterTest, PasswordFileWriteAuthSuccessful) {
  mock_reauthentication_module_.shouldSucceed = YES;
  NSURL* passwordsFileURL = GetPasswordsFileURL();

  OCMExpect([password_exporter_delegate_
      showActivityViewWithActivityItems:[OCMArg isEqual:@[ passwordsFileURL ]]
                      completionHandler:[OCMArg any]]);

  [password_exporter_ startExportFlow:CreatePasswordList()];

  // Wait for all asynchronous tasks to complete.
  scoped_task_environment_.RunUntilIdle();

  EXPECT_TRUE(PasswordFileExists());
  EXPECT_OCMOCK_VERIFY(password_exporter_delegate_);
}

TEST_F(PasswordExporterTest, PasswordFileWriteAuthFailed) {
  mock_reauthentication_module_.shouldSucceed = NO;
  [[password_exporter_delegate_ reject]
      showActivityViewWithActivityItems:[OCMArg any]
                      completionHandler:[OCMArg any]];

  // Use @try/@catch as -reject raises an exception.
  @try {
    [password_exporter_ startExportFlow:CreatePasswordList()];
    scoped_task_environment_.RunUntilIdle();
    EXPECT_OCMOCK_VERIFY(password_exporter_delegate_);
  } @catch (NSException* exception) {
    // The exception is raised when
    // - attemptReauthWithLocalizedReason:canReusePreviousAuth:handler:
    // is invoked. As this should not happen, mark the test as failed.
    GTEST_FAIL();
  }
  EXPECT_FALSE(PasswordFileExists());
}

TEST_F(PasswordExporterTest, PasswordFileNotWrittenUntilSerializationFinished) {
  mock_reauthentication_module_.shouldSucceed = YES;
  // NSURL* passwordsFileURL = GetPasswordsFileURL();
  FakePasswordSerialzerBridge* fakePasswordSerializerBridge =
      [[FakePasswordSerialzerBridge alloc] init];
  [password_exporter_ setPasswordSerializerBridge:fakePasswordSerializerBridge];

  [[password_exporter_delegate_ reject]
      showActivityViewWithActivityItems:[OCMArg any]
                      completionHandler:[OCMArg any]];

  // Use @try/@catch as -reject raises an exception.
  @try {
    [password_exporter_ startExportFlow:CreatePasswordList()];
    scoped_task_environment_.RunUntilIdle();
    EXPECT_OCMOCK_VERIFY(password_exporter_delegate_);
  } @catch (NSException* exception) {
    // The exception is raised when
    // - attemptReauthWithLocalizedReason:canReusePreviousAuth:handler:
    // is invoked. As this should not happen, mark the test as failed.
    GTEST_FAIL();
  }

  EXPECT_FALSE(PasswordFileExists());
  EXPECT_OCMOCK_VERIFY(password_exporter_delegate_);

  OCMExpect([password_exporter_delegate_
      showActivityViewWithActivityItems:[OCMArg isEqual:@[ passwordsFileURL ]]
                      completionHandler:[OCMArg any]]);

  // Finish serialization
  [fakePasswordSerializerBridge executeHandler];

  scoped_task_environment_.RunUntilIdle();

  EXPECT_TRUE(PasswordFileExists());
  EXPECT_OCMOCK_VERIFY(password_exporter_delegate_);
}

}  // namespace
