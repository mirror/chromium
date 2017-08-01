// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/profile_network_context_service.h"

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_features.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"

namespace {

content::mojom::NetworkContextParamsPtr CreateMainNetworkContextParams(
    Profile* profile) {
  // TODO(mmenke): Set up parameters here.
  content::mojom::NetworkContextParamsPtr network_context_params =
      content::mojom::NetworkContextParams::New();
  PrefService* prefs = profile->GetPrefs();

  // Always enable the HTTP cache.
  network_context_params->http_cache_enabled = true;

  // Configure the HTTP cache path and size for non-OTR profiles. OTR profiles
  // just use an in-memory cache with the default size.
  if (!profile->IsOffTheRecord()) {
    base::FilePath base_cache_path;
    chrome::GetUserCacheDirectory(profile->GetPath(), &base_cache_path);
    base::FilePath disk_cache_dir = prefs->GetFilePath(prefs::kDiskCacheDir);
    if (!disk_cache_dir.empty())
      base_cache_path = disk_cache_dir.Append(base_cache_path.BaseName());
    network_context_params->http_cache_path =
        base_cache_path.Append(chrome::kCacheDirname);
    network_context_params->http_cache_max_size =
        prefs->GetInteger(prefs::kDiskCacheSize);
  }

  // NOTE(mmenke): Keep these protocol handlers and
  // ProfileIOData::SetUpJobFactoryDefaultsForBuilder in sync with
  // ProfileIOData::IsHandledProtocol().
  // TODO(mmenke): Find a better way of handling tracking supported schemes.
  network_context_params->enable_data_url_support = true;
  network_context_params->enable_file_url_support = true;
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)

  return network_context_params;
}

}  // namespace

ProfileNetworkContextService::ProfileNetworkContextService(Profile* profile)
    : profile_(profile),
      profile_io_data_context_request_(
          mojo::MakeRequest(&profile_io_data_main_network_context_)) {}

ProfileNetworkContextService::~ProfileNetworkContextService() {
  // Some interactive_ui_tests use a real ChromeContentBrowserClient with a
  // TestingProfile, which results in CreateMainNetworkContext() being called
  // but not SetUpProfileIODataMainContext(). InterfaceRequests DCHECK on
  // destruction when pending by default, so need this to avoid DCHECKing in
  // those tests.
  //
  // TODO(mmenke): Remove this once ProfileIOData no longer calls
  // SetUpProfileIODataMainContext(). This will probably only be the case when
  // ProfileIOData is removed.
  if (profile_io_data_context_request_.is_pending())
    profile_io_data_context_request_.ResetWithReason(0u, "Tests are weird");
}

void ProfileNetworkContextService::SetUpProfileIODataMainContext(
    content::mojom::NetworkContextRequest* network_context_request,
    content::mojom::NetworkContextParamsPtr* network_context_params) {
  DCHECK(network_context_request);
  DCHECK(network_context_params);
  DCHECK(profile_io_data_context_request_.is_pending());

  *network_context_request = std::move(profile_io_data_context_request_);

  if (!base::FeatureList::IsEnabled(features::kNetworkService)) {
    *network_context_params = CreateMainNetworkContextParams(profile_);
    return;
  }

  // Just use default if network service is enabled, to avoid the legacy
  // in-process URLRequestContext from fighting with the NetworkService over
  // ownership of on-disk files.
  *network_context_params = content::mojom::NetworkContextParams::New();
}

content::mojom::NetworkContextPtr
ProfileNetworkContextService::CreateMainNetworkContext() {
  DCHECK(profile_io_data_main_network_context_);

  if (!base::FeatureList::IsEnabled(features::kNetworkService))
    return std::move(profile_io_data_main_network_context_);

  content::mojom::NetworkContextPtr network_context;
  content::GetNetworkService()->CreateNetworkContext(
      MakeRequest(&network_context), CreateMainNetworkContextParams(profile_));
  return network_context;
}
