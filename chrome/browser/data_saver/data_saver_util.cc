// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_saver/data_saver_util.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/pref_names.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace chrome {

// static
bool WouldLikelyBeFetchedViaDataSaverIO(ProfileIOData* profile_io_data,
                                        const GURL& gurl) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  data_reduction_proxy::DataReductionProxyIOData* data_reduction_proxy_io_data =
      profile_io_data->data_reduction_proxy_io_data();

  if (!data_reduction_proxy_io_data)
    return false;

  if (!data_reduction_proxy_io_data->IsEnabled())
    return false;

  if (!gurl.is_valid() || !gurl.SchemeIs(url::kHttpScheme)) {
    // Only HTTP URLs are fetched via data saver.
    return false;
  }

  return true;
}

}  // namespace chrome
