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

std::unique_ptr<IndependentOTRProfileManager::OTRProfileRegistration>
IndependentOTRProfileManager::CreateFromOriginalProfile(
    Profile* profile,
    ProfileDestroyedCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  DCHECK(!callback.is_null());
  auto* otr_profile = profile->CreateOffTheRecordProfile();
  auto entry = refcounts_.emplace(otr_profile, 1);
  auto callback_entry = callbacks_.emplace(otr_profile, std::move(callback));
  DCHECK(entry.second);
  DCHECK(callback_entry.second);
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::Source<Profile>(profile));
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
    registrar_.Remove(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                      content::Source<Profile>(profile->GetOriginalProfile()));
    auto callback_entry = callbacks_.find(profile);
    if (callback_entry != callbacks_.end())
      callbacks_.erase(callback_entry);
    // This will be called from within ~Browser so we can't delete its profile
    // immediately in case it has a (possibly indirect) dependency on the
    // profile.
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, entry->first);
    refcounts_.erase(entry);
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

void IndependentOTRProfileManager::UnregisterProfile(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto entry = refcounts_.find(profile);
  DCHECK(entry != refcounts_.end());
  if (entry->second == 1) {
    registrar_.Remove(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                      content::Source<Profile>(profile->GetOriginalProfile()));
    auto callback_entry = callbacks_.find(profile);
    if (callback_entry != callbacks_.end())
      callbacks_.erase(callback_entry);
    delete entry->first;
    refcounts_.erase(entry);
  } else {
    --entry->second;
  }
}

void IndependentOTRProfileManager::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_PROFILE_DESTROYED, type);
  auto callback_entry = std::find_if(
      callbacks_.begin(), callbacks_.end(), [source](const auto& entry) {
        return source ==
               content::Source<Profile>(entry.first->GetOriginalProfile());
      });
  DCHECK(callback_entry != callbacks_.end());
  // If the registration is destroyed from within this callback, we don't want
  // to double-erase.  If it isn't, we still need to erase the callback entry.
  auto* profile = callback_entry->first;
  auto callback = std::move(callback_entry->second);
  callbacks_.erase(callback_entry);
  std::move(callback).Run(profile);
}
