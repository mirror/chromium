// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_UPDATER_UPDATE_SERVICE_H_
#define EXTENSIONS_BROWSER_UPDATER_UPDATE_SERVICE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class Version;
}

namespace content {
class BrowserContext;
}

namespace update_client {
enum class Error;
class UpdateClient;
}

namespace extensions {

class ExtensionCache;
class UpdateDataProvider;
class UpdateServiceFactory;

// This service manages the autoupdate of extensions.  It should eventually
// replace ExtensionUpdater in Chrome.
// TODO(rockot): Replace ExtensionUpdater with this service.
class UpdateService : public KeyedService {
 public:
  struct ExtensionUpdateInfo {
    ExtensionUpdateInfo();
    ExtensionUpdateInfo(const ExtensionUpdateInfo& other);
    ~ExtensionUpdateInfo();

    std::string extension_id;
    std::string install_source;
    bool is_corrupt_reinstall;
  };

  struct UpdateCheckParams {
    enum UpdateCheckPriority {
      BACKGROUND,
      FOREGROUND,
    };
    UpdateCheckParams();
    UpdateCheckParams(const UpdateCheckParams& other);
    ~UpdateCheckParams();

    std::vector<ExtensionUpdateInfo> update_info;
    UpdateCheckPriority priority;
  };

  static UpdateService* Get(content::BrowserContext* context);

  void Shutdown() override;

  void SendUninstallPing(const std::string& id,
                         const base::Version& version,
                         int reason);

  // Starts an update check for each of |extension_ids|. If there are any
  // updates available, they will be downloaded, checked for integrity,
  // unpacked, and then passed off to the ExtensionSystem::InstallUpdate method
  // for install completion.
  void StartUpdateCheck(const UpdateCheckParams& params);

  // This function verifies if the current implementation can update
  // |extension_id|.
  bool CanUpdate(const std::string& extension_id) const;

 private:
  friend class UpdateServiceFactory;
  friend std::unique_ptr<UpdateService>::deleter_type;

  UpdateService(content::BrowserContext* context,
                ExtensionCache* cache,
                scoped_refptr<update_client::UpdateClient> update_client);
  ~UpdateService() override;

  void UpdateCheckComplete(const std::vector<std::string>& extension_ids,
                           update_client::Error error);
  void DoUpdateCheck(const UpdateCheckParams& params);

  content::BrowserContext* context_;
  ExtensionCache* extension_cache_;

  scoped_refptr<update_client::UpdateClient> update_client_;
  scoped_refptr<UpdateDataProvider> update_data_provider_;

  // The set of extensions that are being checked for update.
  std::set<std::string> updating_extensions_;

  // used to create WeakPtrs to |this|.
  base::WeakPtrFactory<UpdateService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(UpdateService);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_UPDATER_UPDATE_SERVICE_H_
