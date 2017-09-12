// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/lock_screen_apps/fake_profile_loader.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/test/base/testing_profile_manager.h"

namespace lock_screen_apps {

FakeProfileLoader::FakeProfileLoader(TestingProfileManager* profile_manager)
    : profile_manager_(profile_manager) {}

FakeProfileLoader::~FakeProfileLoader() {}

void FakeProfileLoader::SetLoadingProfile() {
  OnLockScreenProfileLoadStarted();
}

void FakeProfileLoader::CreateProfile() {
  if (!LoadingProfile())
    OnLockScreenProfileLoadStarted();

  Profile* profile = profile_manager_->CreateTestingProfile(
      chromeos::ProfileHelper::GetLockScreenAppProfileName());

  extensions::TestExtensionSystem* extension_system =
      static_cast<extensions::TestExtensionSystem*>(
          extensions::ExtensionSystem::Get(profile));
  extension_system->CreateExtensionService(
      base::CommandLine::ForCurrentProcess(),
      base::FilePath() /* install_directory */, false /* autoupdate_enabled */);

  OnLockScreenProfileLoaded(profile);
}

void FakeProfileLoader::SetProfileCreationFailed() {
  if (!LoadingProfile())
    OnLockScreenProfileLoadStarted();
  OnLockScreenProfileLoaded(nullptr);
}

void FakeProfileLoader::InitializeImpl() {}

}  // namespace lock_screen_apps
