// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/independent_otr_profile_manager.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"

using content::BrowserThread;

IndependentOTRProfileManager::OTRProfileRegistration::
    ~OTRProfileRegistration() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  manager_->UnregisterProfile(profile_);
}

IndependentOTRProfileManager::OTRProfileRegistration::OTRProfileRegistration(
    IndependentOTRProfileManager* manager,
    Profile* profile)
    : manager_(manager), profile_(profile) {
  DCHECK(manager_);
  DCHECK(profile);
  DCHECK(profile->IsOffTheRecord());
}

// static
IndependentOTRProfileManager* IndependentOTRProfileManager::GetInstance() {
  static IndependentOTRProfileManager manager;
  return &manager;
}

std::unique_ptr<IndependentOTRProfileManager::OTRProfileRegistration>
IndependentOTRProfileManager::CreateFromOriginalProfile(
    Profile* profile,
    ProfileDestroyedCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  DCHECK(!callback.is_null());
  if (!HasDependentProfiles(profile)) {
    registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                   content::Source<Profile>(profile));
  }
  auto* otr_profile = profile->CreateOffTheRecordProfile();
  auto entry = refcounts_.emplace(otr_profile, 1);
  auto callback_entry = callbacks_.emplace(otr_profile, std::move(callback));
  DCHECK(entry.second);
  DCHECK(callback_entry.second);
  return base::WrapUnique(new OTRProfileRegistration(this, otr_profile));
}

void IndependentOTRProfileManager::OnBrowserAdded(Browser* browser) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = browser->profile();
  auto entry = refcounts_.find(profile);
  if (entry != refcounts_.end()) {
    ++entry->second;
  }
}

void IndependentOTRProfileManager::OnBrowserRemoved(Browser* browser) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = browser->profile();
  auto entry = refcounts_.find(profile);
  if (entry == refcounts_.end()) {
    return;
  }
  if (entry->second == 1) {
    DCHECK(callbacks_.find(profile) == callbacks_.end());
    // This will be called from within ~Browser so we can't delete its profile
    // immediately in case it has a (possibly indirect) dependency on the
    // profile.
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, entry->first);
    auto* original_profile = entry->first->GetOriginalProfile();
    refcounts_.erase(entry);
    if (!HasDependentProfiles(original_profile)) {
      registrar_.Remove(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                        content::Source<Profile>(original_profile));
    }
  } else {
    --entry->second;
  }
}

IndependentOTRProfileManager::IndependentOTRProfileManager() {
  BrowserList::AddObserver(this);
}

IndependentOTRProfileManager::~IndependentOTRProfileManager() {
  BrowserList::RemoveObserver(this);
}

bool IndependentOTRProfileManager::HasDependentProfiles(
    Profile* profile) const {
  return std::find_if(refcounts_.begin(), refcounts_.end(),
                      [profile](const std::pair<Profile*, int32_t>& entry) {
                        return entry.first->GetOriginalProfile() == profile;
                      }) != refcounts_.end();
}

void IndependentOTRProfileManager::UnregisterProfile(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto entry = refcounts_.find(profile);
  DCHECK(entry != refcounts_.end());
  callbacks_.erase(profile);
  if (entry->second == 1) {
    auto* original_profile = profile->GetOriginalProfile();
    delete entry->first;
    refcounts_.erase(entry);
    if (!HasDependentProfiles(original_profile)) {
      registrar_.Remove(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                        content::Source<Profile>(original_profile));
    }
  } else {
    --entry->second;
  }
}

void IndependentOTRProfileManager::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
  for (auto it = callbacks_.begin(); it != callbacks_.end();) {
    if (source != content::Source<Profile>(it->first->GetOriginalProfile())) {
      ++it;
      continue;
    }
    // If the registration is destroyed from within this callback, we don't want
    // to double-erase.  If it isn't, we still need to erase the callback entry.
    auto* profile = it->first;
    auto callback = std::move(it->second);
    it = callbacks_.erase(it);
    std::move(callback).Run(profile);
  }
}
