// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/data_deleter.h"

#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_special_storage_policy.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/api/storage/storage_frontend.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/app_isolation_info.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using base::WeakPtr;
using content::BrowserContext;
using content::BrowserThread;
using content::StoragePartition;

namespace extensions {

namespace {

// Provide a callback wrapper that delays calling the passed callback
// until the wrapper has been called a number of times specified in the
// constructor.  This class owns itself and will delete itself after the
// specified number of calls.
class CallbackWrapper {
 public:
  CallbackWrapper(base::OnceClosure callback, int wraps)
      : callback_(std::move(callback)), wraps_(wraps) {}

  base::Closure WrappingCallback() {
    return base::Bind(&CallbackWrapper::WrappingFunction,
                      base::Unretained(this));
  }

 private:
  void WrappingFunction() {
    DCHECK_GT(wraps_, 0);
    if (--wraps_ == 0) {
      std::move(callback_).Run();
      delete this;
    }
  }

  base::OnceClosure callback_;
  int wraps_;
};

bool DoesCookieMatchHost(const std::string& host,
                         const net::CanonicalCookie& cookie) {
  return cookie.IsHostCookie() && cookie.IsDomainMatch(host);
}

void OnClearedCookies(base::OnceClosure callback, uint32_t /* num_deleted */) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, std::move(callback));
}

void ClearCookiesOnIOThread(scoped_refptr<net::URLRequestContextGetter> context,
                            GURL origin,
                            base::OnceClosure ui_callback) {
  net::CookieStore* cookie_store =
      context->GetURLRequestContext()->cookie_store();
  cookie_store->DeleteAllCreatedBetweenWithPredicateAsync(
      base::Time(), base::Time::Max(),
      base::Bind(&DoesCookieMatchHost, origin.host()),
      base::BindOnce(&OnClearedCookies, std::move(ui_callback)));
}

// Helper function that deletes data of a given |storage_origin| in a given
// |partition|.
void DeleteOrigin(Profile* profile,
                  StoragePartition* partition,
                  const GURL& origin,
                  const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(profile);
  DCHECK(partition);

  if (origin.SchemeIs(kExtensionScheme)) {
    // TODO(ajwong): Cookies are not properly isolated for
    // chrome-extension:// scheme.  (http://crbug.com/158386).
    //
    // However, no isolated apps actually can write to kExtensionScheme
    // origins. Thus, it is benign to delete from the
    // RequestContextForExtensions because there's nothing stored there. We
    // preserve this code path without checking for isolation because it's
    // simpler than special casing.  This code should go away once we merge
    // the various URLRequestContexts (http://crbug.com/159193).

    // Self-owned.
    CallbackWrapper* wrapper = new CallbackWrapper(callback, 2);

    partition->ClearDataForOrigin(
        ~StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE,
        StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL, origin,
        wrapper->WrappingCallback());

    // Delete cookies separately from other data so that the request context
    // for extensions doesn't need to be passed into the StoragePartition.
    // TODO(rdsmith): Mojoify this call and get rid of the thread hopping.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &ClearCookiesOnIOThread,
            // Safe not to refcount because deletion of the
            // request context getter happens on the IO thread and
            // thus must occur after the execution of this PostTask.
            base::WrapRefCounted(profile->GetRequestContextForExtensions()),
            origin, wrapper->WrappingCallback()));
  } else {
    // We don't need to worry about the media request context because that
    // shares the same cookie store as the main request context.
    partition->ClearDataForOrigin(
        ~StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE,
        StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL,
        origin,
        callback);
  }
}

void OnNeedsToGarbageCollectIsolatedStorage(WeakPtr<ExtensionService> es,
                                            const base::Closure& callback) {
  if (es)
    ExtensionPrefs::Get(es->profile())->SetNeedsStorageGarbageCollection(true);
  callback.Run();
}

}  // namespace

// static
void DataDeleter::StartDeleting(Profile* profile,
                                const Extension* extension,
                                const base::Closure& callback) {
  DCHECK(profile);
  DCHECK(extension);

  if (AppIsolationInfo::HasIsolatedStorage(extension)) {
    BrowserContext::AsyncObliterateStoragePartition(
        profile,
        util::GetSiteForExtensionId(extension->id(), profile),
        base::Bind(
            &OnNeedsToGarbageCollectIsolatedStorage,
            ExtensionSystem::Get(profile)->extension_service()->AsWeakPtr(),
            callback));
  } else {
    GURL launch_web_url_origin(
        AppLaunchInfo::GetLaunchWebURL(extension).GetOrigin());

    StoragePartition* partition = BrowserContext::GetStoragePartitionForSite(
        profile,
        Extension::GetBaseURLFromExtensionId(extension->id()));

    ExtensionSpecialStoragePolicy* storage_policy =
        profile->GetExtensionSpecialStoragePolicy();
    if (storage_policy->NeedsProtection(extension) &&
        !storage_policy->IsStorageProtected(launch_web_url_origin)) {
      DeleteOrigin(profile,
                   partition,
                   launch_web_url_origin,
                   base::Bind(&base::DoNothing));
    }
    DeleteOrigin(profile, partition, extension->url(), callback);
  }

  // Begin removal of the settings for the current extension.
  // StorageFrontend may not exist in unit tests.
  StorageFrontend* frontend = StorageFrontend::Get(profile);
  if (frontend)
    frontend->DeleteStorageSoon(extension->id());
}

}  // namespace extensions
