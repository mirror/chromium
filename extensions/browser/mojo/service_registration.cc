// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mojo/service_registration.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/mojo/keep_alive_impl.h"
#include "extensions/browser/process_map.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/switches.h"
#include "extensions/features/features.h"
#include "services/service_manager/public/cpp/binder_registry.h"

#if BUILDFLAG(ENABLE_WIFI_DISPLAY)
#include "extensions/browser/api/display_source/wifi_display/wifi_display_media_service_impl.h"
#include "extensions/browser/api/display_source/wifi_display/wifi_display_session_service_impl.h"
#endif

namespace extensions {
namespace {

#if BUILDFLAG(ENABLE_WIFI_DISPLAY)
bool ExtensionHasPermission(const Extension* extension,
                            content::RenderProcessHost* render_process_host,
                            const std::string& permission_name) {
  Feature::Context context =
      ProcessMap::Get(render_process_host->GetBrowserContext())
          ->GetMostLikelyContextType(extension, render_process_host->GetID());

  return ExtensionAPI::GetSharedInstance()
      ->IsAvailable(permission_name, extension, context, extension->url())
      .is_available();
}

template <typename Interface>
void BindCallback(base::Callback<void(const service_manager::BindSourceInfo >
                                          &mojo::InterfaceRequest<Interface>,
                                      content::RenderFrameHost*)> callback,
                  const service_manager::BindSourceInfo& source_info,
                  mojo::InterfaceRequest<Interface> request,
                  content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  ExtensionWebContentsObserver* observer =
      ExtensionWebContentsObserver::GetForWebContents(web_contents);
  const Extension* extension = GetExtensionFromFrame(render_frame_host, false);
  if (ExtensionHasPermission(extension, render_frame_host->GetProcess(),
                             "displaySource")) {
    callback.Run(source_info, std::move(request), render_frame_host);
  }
}

#endif

}  // namespace

void AddInterfacesForFrameRegistry(Registry* registry) {
  registry->AddInterface(base::Bind(KeepAliveImpl::Create));
#if BUILDFLAG(ENABLE_WIFI_DISPLAY)
  registry->AddInterface(
      base::Bind(&BindCallback<extensions::WiFiDisplaySessionServiceImpl>,
                 base::Bind(&WiFiDisplaySessionServiceImpl::BindToRequest)));
  registry->AddInterface(
      base::Bind(&BindCallback<extensions::WiFiDisplaySessionServiceImpl>,
                 base::Bind(&WiFiDisplayMediaServiceImpl::BindToRequest)));
#endif
}

}  // namespace extensions
