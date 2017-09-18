// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_id.h"

#include "base/callback.h"
#include "chrome/browser/media/media_storage_id_salt.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "url/origin.h"

namespace chrome {

namespace {

std::vector<uint8_t> GetSalt(content::RenderFrameHost* rfh) {
  DCHECK(rfh);
  Profile* profile =
      Profile::FromBrowserContext(rfh->GetProcess()->GetBrowserContext());
  return MediaStorageIdSalt::GetSalt(profile->GetPrefs());
}

}  // namespace

void ComputeStorageId(content::RenderFrameHost* rfh,
                      CdmStorageIdCallback callback) {
  DCHECK(rfh);
  std::vector<uint8_t> salt = GetSalt(rfh);
  url::Origin origin = rfh->GetLastCommittedOrigin();

  // TODO(jrummell): Implement the details. http://crbug.com/478960.
  DCHECK(salt.size());
  DCHECK(!origin.unique());
  std::move(callback).Run(std::vector<uint8_t>());
}

}  // namespace chrome
