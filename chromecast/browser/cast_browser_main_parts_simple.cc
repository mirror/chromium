// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/memory/ptr_util.h"
#include "chromecast/browser/cast_browser_main_parts.h"

namespace chromecast {
namespace shell {

// static
CastBrowserMainParts* CastBrowserMainParts::Create(
    const content::MainFunctionParams& parameters,
    URLRequestContextFactory* url_request_context_factory);
return new CastBrowserMainParts(parameters, url_request_context_factory);
}  // namespace shell

}  // namespace chromecast
}  // namespace chromecast
