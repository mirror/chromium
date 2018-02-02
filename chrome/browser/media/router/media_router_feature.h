// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FEATURE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FEATURE_H_

#include "build/build_config.h"

#if !defined(OS_ANDROID)
#include "base/feature_list.h"
#include "extensions/common/extension_id.h"
#endif  // !defined(OS_ANDROID)

namespace content {
class BrowserContext;
}

namespace media_router {

// Returns true if Media Router is enabled for |context|.
bool MediaRouterEnabled(content::BrowserContext* context);

#if !defined(OS_ANDROID)

extern const base::Feature kEnableDialLocalDiscovery;
extern const base::Feature kEnableCastDiscovery;
extern const base::Feature kEnableCastLocalMedia;

// Returns true if browser side DIAL discovery is enabled.
bool DialLocalDiscoveryEnabled();

// Returns true if browser side Cast discovery is enabled.
bool CastDiscoveryEnabled();

// Returns true if local media casting is enabled.
bool CastLocalMediaEnabled();

// Returns true if the presentation receiver window for local media casting is
// available on the current platform.
// TODO(crbug.com/802332): Remove this when mac_views_browser=1 by default.
bool PresentationReceiverWindowEnabled();

// Returns the extension ID of the Media Router component extension to load, or
// the empty string if no extension is to be loaded.
extensions::ExtensionId GetMediaRouterComponentExtensionId();

// Returns true if |extension_id| matches the external or internal component
// extension ID, respectively.
bool IsMediaRouterExternalComponent(const extensions::ExtensionId extension_id);
bool IsMediaRouterInternalComponent(const extensions::ExtensionId extension_id);

#endif  // !defined(OS_ANDROID)

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FEATURE_H_
