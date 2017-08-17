// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/optional.h"
#include "components/arc/common/file_system.mojom.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace storage {
class FileSystemURL;
}  // namespace storage

namespace arc {

class ArcDocumentsProviderRoot;

// Container of ArcDocumentsProviderRoot instances.
//
// All member function must be called on the UI thread.
class ArcDocumentsProviderRootMap : public KeyedService {
 public:
  using RefreshCallback = base::Callback<
    void(const base::Optional<std::vector<mojom::RootPtr>>& roots)>;
  ~ArcDocumentsProviderRootMap() override;

  // Returns an instance for the given browser context, or nullptr if ARC is not
  // allowed for the browser context.
  static ArcDocumentsProviderRootMap* GetForBrowserContext(
      content::BrowserContext* context);

  // Returns an instance for the browser context associated with ARC, or nullptr
  // if ARC is not allowed.
  // TODO(nya): Remove this function when we support multi-user ARC. For now,
  // it is okay to call this function only from chromeos::FileSystemBackend and
  // its delegates.
  static ArcDocumentsProviderRootMap* GetForArcBrowserContext();

  // Looks up a root corresponding to |url|.
  // |path| is set to the remaining path part of |url|.
  // Returns nullptr if |url| is invalid or no corresponding root is registered.
  ArcDocumentsProviderRoot* ParseAndLookup(const storage::FileSystemURL& url,
                                           base::FilePath* path) const;

  // KeyedService overrides:
  void Shutdown() override;
  // Refreshes the roots list and returns it.
  // Call from UI thread.
  void Refresh(const RefreshCallback& callback);

 private:
  friend class ArcDocumentsProviderRootMapFactory;

  explicit ArcDocumentsProviderRootMap(Profile* profile);

  void RefreshInternal_(
    const RefreshCallback& callback,
    base::Optional<std::vector<mojom::RootPtr>> roots);

  // Key is (authority, root_document_id).
  using Key = std::pair<std::string, std::string>;
  std::map<Key, std::unique_ptr<ArcDocumentsProviderRoot>> map_;
  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(ArcDocumentsProviderRootMap);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_H_
