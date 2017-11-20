// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/independent_otr_profile_manager.h"

#include <utility>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

IndependentOTRProfileManager::OTRProfileRegistration::OTRProfileRegistration(
    IndependentOTRProfileManager::OTRProfileRegistration&& other)
    : manager_(other.manager_), profile_(other.profile_) {
  other.manager_ = nullptr;
  other.profile_ = nullptr;
}

IndependentOTRProfileManager::OTRProfileRegistration&
IndependentOTRProfileManager::OTRProfileRegistration::operator=(
    IndependentOTRProfileManager::OTRProfileRegistration other) {
  using std::swap;
  swap(manager_, other.manager_);
  swap(profile_, other.profile_);
  return *this;
}

IndependentOTRProfileManager::OTRProfileRegistration::
    ~OTRProfileRegistration() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (manager_) {
    DCHECK(profile_);
    manager_->UnregisterProfile(profile_);
  }
}

IndependentOTRProfileManager::OTRProfileRegistration::OTRProfileRegistration(
    IndependentOTRProfileManager* manager,
    Profile* profile)
    : manager_(manager), profile_(profile) {
  DCHECK(profile);
  DCHECK(profile->IsOffTheRecord());
}

// static
IndependentOTRProfileManager* IndependentOTRProfileManager::GetInstance() {
  static IndependentOTRProfileManager manager;
  return &manager;
}

IndependentOTRProfileManager::OTRProfileRegistration
IndependentOTRProfileManager::CreateFromOriginalProfile(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  auto* otr_profile = profile->CreateOffTheRecordProfile();
  auto entry = refcounts_.emplace(otr_profile, 1);
  DCHECK(entry.second);
  return {this, otr_profile};
}

bool IndependentOTRProfileManager::IsIndependentOTRProfile(
    Profile* profile) const {
  return base::ContainsKey(refcounts_, profile);
}

void IndependentOTRProfileManager::OnBrowserAdded(Browser* browser) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = browser->profile();
  auto entry = refcounts_.find(profile);
  if (entry != refcounts_.end()) {
    ++entry->second;
  }
}

namespace {

void DeleteProfile(Profile* profile) {
  delete profile;
}

}  // namespace

void IndependentOTRProfileManager::OnBrowserRemoved(Browser* browser) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = browser->profile();
  auto entry = refcounts_.find(profile);
  if (entry == refcounts_.end()) {
    return;
  }
  if (entry->second == 1) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&DeleteProfile, entry->first));
    refcounts_.erase(entry);
  } else {
    --entry->second;
  }
}

IndependentOTRProfileManager::IndependentOTRProfileManager() {
  BrowserList::GetInstance()->AddObserver(this);
}

IndependentOTRProfileManager::~IndependentOTRProfileManager() {
  BrowserList::GetInstance()->RemoveObserver(this);
}

void IndependentOTRProfileManager::UnregisterProfile(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto entry = refcounts_.find(profile);
  DCHECK(entry != refcounts_.end());
  if (entry->second == 1) {
    delete entry->first;
    refcounts_.erase(entry);
  } else {
    --entry->second;
  }
}
