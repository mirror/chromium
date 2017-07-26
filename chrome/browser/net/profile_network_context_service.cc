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
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/storage_partition.h"
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

  network_context_params->http_cache_enabled = true;
  if (!profile->IsOffTheRecord()) {
    network_context_params->http_cache_dir = base::FilePath(profile->GetPath());
    base::FilePath cache_subdir = prefs->GetFilePath(prefs::kDiskCacheDir);
    if (!cache_subdir.empty())
      network_context_params->http_cache_dir->Append(cache_subdir);
    network_context_params->http_cache_dir->Append(chrome::kCacheDirname);
    network_context_params->http_cache_max_size = prefs->GetInteger(prefs::kDiskCacheSize);
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
    : profile_(profile) {}

ProfileNetworkContextService::~ProfileNetworkContextService() {}

void ProfileNetworkContextService::SetUpProfileIODataMainContext(
    content::mojom::NetworkContextRequest* network_context_request,
    content::mojom::NetworkContextParamsPtr* network_context_params) {
  DCHECK(!profile_io_data_main_network_context_);
  *network_context_request =
      mojo::MakeRequest(&profile_io_data_main_network_context_);
  if (!base::FeatureList::IsEnabled(features::kNetworkService)) {
    *network_context_params = CreateMainNetworkContextParams(profile_);
  } else {
    // Just use default if network service is enabled, to avoid the legacy
    // in-process URLRequestContext from fighting with the NetworkService over
    // ownership of on-disk files.
    *network_context_params = content::mojom::NetworkContextParams::New();
  }
}

content::mojom::NetworkContext* ProfileNetworkContextService::MainContext() {
  // ProfileIOData must be initialized before this call.
  DCHECK(profile_io_data_main_network_context_);
  if (!base::FeatureList::IsEnabled(features::kNetworkService))
    return profile_io_data_main_network_context_.get();

  return content::BrowserContext::GetDefaultStoragePartition(profile_)
      ->GetNetworkContext();
}

content::mojom::NetworkContextPtr
ProfileNetworkContextService::CreateMainNetworkContext() {
  DCHECK(base::FeatureList::IsEnabled(features::kNetworkService));

  content::mojom::NetworkContextPtr network_context;
  content::GetNetworkService()->CreateNetworkContext(
      MakeRequest(&network_context), CreateMainNetworkContextParams(profile_));
  return network_context;
}
