// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/predictor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "jni/ProfileManagerUtils_jni.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/interfaces/cookie_manager.mojom.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace {

void FlushCookiesOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> getter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  getter->GetURLRequestContext()->cookie_store()->FlushStore(base::Closure());
}

void FlushStoragePartition(content::StoragePartition* partition) {
  partition->Flush();
}

void CommitPendingWritesForProfile(Profile* profile) {
  // These calls are asynchronous. They may not finish (and may not even
  // start!) before the Android OS kills our process. But we can't wait for them
  // to finish because blocking the UI thread is illegal.
  profile->GetNetworkPredictor()->SaveStateForNextStartup();
  profile->GetPrefs()->CommitPendingWrite();
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&FlushCookiesOnIOThread,
                 base::WrapRefCounted(profile->GetRequestContext())));
  content::BrowserContext::ForEachStoragePartition(
      profile, base::Bind(FlushStoragePartition));
}

void RemoveSessionCookiesOnIOThread(
    network::mojom::CookieManager* cookie_manager) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  network::mojom::CookieDeletionFilterPtr filter(
      network::mojom::CookieDeletionFilter::New());
  filter->session_control =
      network::mojom::CookieDeletionSessionControl::SESSION_COOKIES;
  cookie_manager->DeleteCookies(
      std::move(filter),
      network::mojom::CookieManager::DeleteCookiesCallback());
}

void RemoveSessionCookiesForProfile(Profile* profile) {
  network::mojom::CookieManager* cookie_manager =
      content::BrowserContext::GetDefaultStoragePartition(profile)
          ->GetCookieManagerForBrowserProcess();
  if (cookie_manager) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&RemoveSessionCookiesOnIOThread, cookie_manager));
  }
}

}  // namespace

static void JNI_ProfileManagerUtils_FlushPersistentDataForAllProfiles(
    JNIEnv* env,
    const JavaParamRef<jclass>& obj) {
  std::vector<Profile*> loaded_profiles =
      g_browser_process->profile_manager()->GetLoadedProfiles();
  std::for_each(loaded_profiles.begin(), loaded_profiles.end(),
                CommitPendingWritesForProfile);

  if (g_browser_process->local_state())
    g_browser_process->local_state()->CommitPendingWrite();
}

static void JNI_ProfileManagerUtils_RemoveSessionCookiesForAllProfiles(
    JNIEnv* env,
    const JavaParamRef<jclass>& obj) {
  std::vector<Profile*> loaded_profiles =
      g_browser_process->profile_manager()->GetLoadedProfiles();
  std::for_each(loaded_profiles.begin(), loaded_profiles.end(),
                RemoveSessionCookiesForProfile);
}
