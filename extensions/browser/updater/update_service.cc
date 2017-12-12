// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/updater/update_service.h"

#include <map>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "components/update_client/update_client.h"
#include "components/update_client/update_client_errors.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/updater/extension_cache.h"
#include "extensions/browser/updater/update_data_provider.h"
#include "extensions/browser/updater/update_service_factory.h"

namespace {

using UpdateClientCallback =
    extensions::UpdateDataProvider::UpdateClientCallback;

void SendUninstallPingCompleteCallback(update_client::Error error) {}

void InstallUpdateCallback(content::BrowserContext* context,
                           const std::string& extension_id,
                           const std::string& public_key,
                           const base::FilePath& unpacked_dir,
                           UpdateClientCallback update_client_callback) {
  using InstallError = update_client::InstallError;
  using Result = update_client::CrxInstaller::Result;
  extensions::ExtensionSystem::Get(context)->InstallUpdate(
      extension_id, public_key, unpacked_dir,
      base::BindOnce(
          [](UpdateClientCallback update_client_callback, bool success) {
            DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
            std::move(update_client_callback)
                .Run(Result(success ? InstallError::NONE
                                    : InstallError::GENERIC_ERROR));
          },
          std::move(update_client_callback)));
}

}  // namespace

namespace extensions {

UpdateService::ExtensionUpdateInfo::ExtensionUpdateInfo() {}

UpdateService::ExtensionUpdateInfo::ExtensionUpdateInfo(
    const ExtensionUpdateInfo& other) = default;

UpdateService::ExtensionUpdateInfo::~ExtensionUpdateInfo() {}

UpdateService::UpdateCheckParams::UpdateCheckParams()
    : priority(UpdateCheckParams::BACKGROUND) {}

UpdateService::UpdateCheckParams::UpdateCheckParams(
    const UpdateCheckParams& other) = default;

UpdateService::UpdateCheckParams::~UpdateCheckParams() {}

// static
UpdateService* UpdateService::Get(content::BrowserContext* context) {
  return UpdateServiceFactory::GetForBrowserContext(context);
}

void UpdateService::Shutdown() {
  if (update_data_provider_) {
    update_data_provider_->Shutdown();
    update_data_provider_ = nullptr;
  }
  update_client_ = nullptr;
  context_ = nullptr;
}

void UpdateService::SendUninstallPing(const std::string& id,
                                      const base::Version& version,
                                      int reason) {
  DCHECK(update_client_);
  update_client_->SendUninstallPing(
      id, version, reason, base::BindOnce(&SendUninstallPingCompleteCallback));
}

void UpdateService::StartUpdateCheck(const UpdateCheckParams& params) {
  DCHECK(update_client_);
  if (extension_cache_) {
    extension_cache_->Start(base::BindRepeating(
        &UpdateService::DoUpdateCheck, weak_ptr_factory_.GetWeakPtr(), params));
  } else {
    DoUpdateCheck(params);
  }
}

bool UpdateService::CanUpdate(const std::string& extension_id) const {
  return false;
}

UpdateService::UpdateService(
    content::BrowserContext* context,
    ExtensionCache* cache,
    scoped_refptr<update_client::UpdateClient> update_client)
    : context_(context),
      extension_cache_(cache),
      update_client_(update_client),
      weak_ptr_factory_(this) {
  DCHECK(update_client_);
  update_data_provider_ = base::MakeRefCounted<UpdateDataProvider>(
      context_, base::BindOnce(&InstallUpdateCallback));
}

UpdateService::~UpdateService() {}

void UpdateService::DoUpdateCheck(const UpdateCheckParams& params) {
  std::map<std::string, UpdateDataProvider::ExtensionUpdateData> update_data;
  std::vector<std::string> extension_ids;
  for (const ExtensionUpdateInfo& info : params.update_info) {
    if (updating_extensions_.find(info.extension_id) ==
        updating_extensions_.end()) {
      updating_extensions_.insert(info.extension_id);
      extension_ids.push_back(info.extension_id);

      UpdateDataProvider::ExtensionUpdateData extension_data;
      extension_data.install_source = info.install_source;
      extension_data.is_corrupt_reinstall = info.is_corrupt_reinstall;
      update_data[info.extension_id] = extension_data;
    }
  }
  if (extension_ids.size() == 0)
    return;
  if (params.priority == UpdateCheckParams::BACKGROUND) {
    update_client_->Update(
        extension_ids,
        base::BindOnce(&UpdateDataProvider::GetData, update_data_provider_,
                       std::move(update_data)),
        base::BindOnce(&UpdateService::UpdateCheckComplete,
                       weak_ptr_factory_.GetWeakPtr(), extension_ids));
  } else {
    // TODO (mxnguyen): Run the on demand update in batch.
    for (const std::string& extension_id : extension_ids) {
      std::map<std::string, UpdateDataProvider::ExtensionUpdateData>
          update_data_for_extension;
      update_data_for_extension[extension_id] =
          std::move(update_data[extension_id]);
      update_data.erase(extension_id);
      update_client_->Install(
          extension_id,
          base::BindOnce(&UpdateDataProvider::GetData, update_data_provider_,
                         std::move(update_data_for_extension)),
          base::BindOnce(&UpdateService::UpdateCheckComplete,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::vector<std::string>({extension_id})));
    }
  }
}

void UpdateService::UpdateCheckComplete(
    const std::vector<std::string>& extension_ids,
    update_client::Error error) {
  for (const std::string& extension_id : extension_ids) {
    updating_extensions_.erase(extension_id);
  }
}

}  // namespace extensions
