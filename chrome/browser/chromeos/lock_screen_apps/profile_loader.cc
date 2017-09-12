// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/lock_screen_apps/profile_loader.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"

namespace lock_screen_apps {

ProfileLoader::ProfileLoader() {}

ProfileLoader::~ProfileLoader() {}

void ProfileLoader::Initialize() {
  CHECK_EQ(state_, State::kInitial);
  state_ = State::kInitialized;

  InitializeImpl();
}

void ProfileLoader::AddLoadProfileCallback(base::OnceClosure callback) {
  CHECK_NE(state_, State::kInitial);

  if (ProfileLoaded()) {
    std::move(callback).Run();
    return;
  }

  load_profile_callbacks_.emplace_back(std::move(callback));
}

bool ProfileLoader::Initialized() const {
  return state_ != State::kInitial;
}

bool ProfileLoader::LoadingProfile() const {
  return state_ == State::kLoadingProfile;
}

bool ProfileLoader::ProfileLoaded() const {
  return state_ == State::kProfileLoaded;
}

void ProfileLoader::OnLockScreenProfileLoadStarted() {
  CHECK_EQ(State::kInitialized, state_);

  state_ = State::kLoadingProfile;
}

void ProfileLoader::OnLockScreenProfileLoaded(Profile* lock_screen_profile) {
  CHECK_EQ(State::kLoadingProfile, state_);
  state_ = State::kProfileLoaded;

  lock_screen_profile_ = lock_screen_profile;

  while (!load_profile_callbacks_.empty()) {
    std::move(load_profile_callbacks_.front()).Run();
    load_profile_callbacks_.pop_front();
  }
}

}  // namespace lock_screen_apps
