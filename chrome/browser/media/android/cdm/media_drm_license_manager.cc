// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/android/cdm/media_drm_license_manager.h"

#include <vector>

#include "base/bind.h"
#include "base/unguessable_token.h"
#include "components/cdm/browser/media_drm_storage_impl.h"
#include "content/public/browser/browser_thread.h"
#include "media/base/android/media_drm_bridge.h"
#include "url/origin.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

namespace chrome {
namespace {
// Unprovision MediaDrm in IO thread.
void ClearMediaDrmLicensesBlocking(
    std::vector<base::UnguessableToken> origin_ids) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  for (const auto& origin_id : origin_ids) {
    // MediaDrm will unprovision |origin_id| for all security level. Passing
    // DEFAULT here is OK.
    scoped_refptr<media::MediaDrmBridge> media_drm_bridge =
        media::MediaDrmBridge::CreateWithoutSessionSupport(
            kWidevineKeySystem, origin_id.ToString(),
            media::MediaDrmBridge::SECURITY_LEVEL_DEFAULT,
            media::CreateFetcherCB());

    if (!media_drm_bridge) {
      continue;
    }

    media_drm_bridge->Unprovision();
  }
}
}  // namespace

void ClearMediaDrmLicenses(
    PrefService* prefs,
    base::Time delete_begin,
    base::Time delete_end,
    const base::RepeatingCallback<bool(const GURL& url)>& filter,
    base::OnceClosure complete_cb) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Clear persisted license meta data in |prefs|.
  std::vector<base::UnguessableToken> no_license_origin_ids =
      cdm::MediaDrmStorageImpl::ClearLicenses(prefs, delete_begin, delete_end,
                                              filter);

  if (no_license_origin_ids.empty()) {
    std::move(complete_cb).Run();
    return;
  }

  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&ClearMediaDrmLicensesBlocking,
                     std::move(no_license_origin_ids)),
      std::move(complete_cb));
}

}  // namespace chrome
