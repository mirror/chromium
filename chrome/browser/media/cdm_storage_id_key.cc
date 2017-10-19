// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_id_key.h"

#include "media/media_features.h"

#if BUILDFLAG(ENABLE_MEDIA_CDM_STORAGE_ID) && defined(GOOGLE_CHROME_BUILD)
#include "chrome/browser/internal/cdm_storage_id_key.h"
#endif

namespace chrome {

std::string GetCdmStorageIdKey() {
#if defined(CDM_STORAGE_ID_KEY)
  return CDM_STORAGE_ID_KEY;
#else
  return std::string();
#endif
}

}  // namespace chrome
