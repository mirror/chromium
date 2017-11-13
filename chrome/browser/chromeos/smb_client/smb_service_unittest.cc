// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service.h"

#include <stddef.h>

#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/file_system_provider/fake_provided_file_system.h"
#include "chrome/browser/chromeos/file_system_provider/fake_registry.h"
#include "chrome/browser/chromeos/file_system_provider/logging_observer.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/scoped_user_manager_enabler.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace smb_client {

namespace {
const char kDisplayName[] = "Home Samba Server";
const char kFileSystemId[] = "samba/home/";
}  // namespace

class SmbServiceTest : public testing::Test {
 protected:
  SmbServiceTest() : profile_(NULL) {}

  ~SmbServiceTest() override {}

  void SetUp() override {
    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("test-user@example.com");
    user_manager_ = new FakeChromeUserManager();
    user_manager_->AddUser(
        AccountId::FromUserEmail(profile_->GetProfileUserName()));
    user_manager_enabler_.reset(new ScopedUserManagerEnabler(user_manager_));

    // Create service.
    extension_registry_.reset(new extensions::ExtensionRegistry(profile_));
    service_.reset(
        new file_system_provider::Service(profile_, extension_registry_.get()));
    service_->SetDefaultFileSystemFactoryForTesting(
        base::Bind(&file_system_provider::FakeProvidedFileSystem::Create));

    registry_ = new file_system_provider::FakeRegistry;
    // Passes ownership to the service instance.
    service_->SetRegistryForTesting(base::WrapUnique(registry_));

    // Create smb service.
    smb_service_.reset(new SmbService(profile_));
  }

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  TestingProfile* profile_;
  FakeChromeUserManager* user_manager_;
  std::unique_ptr<ScopedUserManagerEnabler> user_manager_enabler_;
  std::unique_ptr<SmbService> smb_service_;

  // Needed to create service
  std::unique_ptr<file_system_provider::Service> service_;
  std::unique_ptr<extensions::ExtensionRegistry> extension_registry_;
  file_system_provider::FakeRegistry* registry_;
};

TEST_F(SmbServiceTest, MountFileSystem) {
  file_system_provider::LoggingObserver observer;
  service_->AddObserver(&observer);

  EXPECT_EQ(base::File::FILE_OK,
            smb_service_->MountSmb(file_system_provider::MountOptions(
                kFileSystemId, kDisplayName)));

  // ASSERT_EQ(1u, observer.mounts.size());

  // EXPECT_EQ("smb", observer.mounts[0].file_system_info().provider_id());
  // EXPECT_EQ(kFileSystemId,
  //           observer.mounts[0].file_system_info().file_system_id());

  service_->RemoveObserver(&observer);
}

}  // namespace smb_client
}  // namespace chromeos
