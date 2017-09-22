// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_CDM_STORAGE_ID_H_
#define CHROME_BROWSER_MEDIA_CDM_STORAGE_ID_H_

#include <stdint.h>

#include <vector>

#include "base/callback_forward.h"

namespace content {
class RenderFrameHost;
}

// This handles computing the Storage Id for platform verification.
namespace chrome {

using CdmStorageIdCallback =
    base::OnceCallback<void(const std::vector<uint8_t>& storage_id)>;

// Computes the Storage Id based on values obtained from |rfh|. This may be
// asynchronous, so call |callback| with the result. If Storage Id is not
// supported on the current platform, an empty vector will be passed to
// |callback|.
void ComputeStorageId(content::RenderFrameHost* rfh,
                      CdmStorageIdCallback callback);

}  // namespace chrome

#endif  // CHROME_BROWSER_MEDIA_CDM_STORAGE_ID_H_
