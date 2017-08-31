// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/log_web_ui_url.h"

#include <stdint.h>

#include "base/hash.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/common/url_constants.h"
#include "content/public/common/url_constants.h"
#include "extensions/features/features.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/extensions/extension_constants.h"
#include "extensions/common/constants.h"
#endif

namespace webui {

const char kWebUICreatedForPath[] = "WebUI.CreatedForPath";
const char kWebUICreatedForUrl[] = "WebUI.CreatedForUrl";

bool LogWebUIUrl(const GURL& web_ui_url) {
  bool should_log = web_ui_url.SchemeIs(content::kChromeUIScheme) ||
                    web_ui_url.SchemeIs(content::kChromeDevToolsScheme);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (web_ui_url.SchemeIs(extensions::kExtensionScheme))
    should_log = web_ui_url.host() == extension_misc::kBookmarkManagerId;
#endif

  if (should_log) {
    // Record the origin.
    // TODO(dschuyler): If the origin and path (below) is sufficient, look into
    // removing this origin only histogram.
    uint32_t hash = base::Hash(web_ui_url.GetOrigin().spec());
    UMA_HISTOGRAM_SPARSE_SLOWLY(kWebUICreatedForUrl,
                                static_cast<base::HistogramBase::Sample>(hash));
    // Record the origin and path.
    GURL::Replacements remove_params;
    remove_params.ClearUsername();
    remove_params.ClearPassword();
    remove_params.ClearQuery();
    remove_params.ClearRef();
    uint32_t path_hash =
        base::Hash(web_ui_url.ReplaceComponents(remove_params).spec());
    UMA_HISTOGRAM_SPARSE_SLOWLY(
        kWebUICreatedForPath,
        static_cast<base::HistogramBase::Sample>(path_hash));
  }

  return should_log;
}

}  // namespace webui
