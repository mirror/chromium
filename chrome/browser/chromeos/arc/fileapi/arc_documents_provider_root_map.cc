// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map_factory.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_service_manager.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace arc {

namespace {

struct DocumentsProviderSpec {
  const char* authority;
  const char* root_document_id;
};

// List of allowed documents providers for production.
constexpr DocumentsProviderSpec kDocumentsProviderWhitelist[] = {
    {"com.android.providers.media.documents", "images_root"},
    {"com.android.providers.media.documents", "videos_root"},
    {"com.android.providers.media.documents", "audio_root"},
};

}  // namespace

// static
ArcDocumentsProviderRootMap* ArcDocumentsProviderRootMap::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcDocumentsProviderRootMapFactory::GetForBrowserContext(context);
}

// static
ArcDocumentsProviderRootMap*
ArcDocumentsProviderRootMap::GetForArcBrowserContext() {
  return GetForBrowserContext(ArcServiceManager::Get()->browser_context());
}

ArcDocumentsProviderRootMap::ArcDocumentsProviderRootMap(Profile* profile)
    : weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  runner_ = ArcFileSystemOperationRunner::GetForBrowserContext(profile);
  DCHECK(runner_);

  // TODO(ryoh): Remove this loop after implementing Android-side codes.
  for (const auto& spec : kDocumentsProviderWhitelist) {
    map_[Key(spec.authority, spec.root_document_id)] =
        base::MakeUnique<ArcDocumentsProviderRoot>(
            runner_, spec.authority, spec.root_document_id, "", "", "",
            std::vector<uint8_t>(), 0);
  }
}

ArcDocumentsProviderRootMap::~ArcDocumentsProviderRootMap() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(map_.empty());
}

ArcDocumentsProviderRoot* ArcDocumentsProviderRootMap::ParseAndLookup(
    const storage::FileSystemURL& url,
    base::FilePath* path) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::string authority;
  std::string root_document_id;
  base::FilePath tmp_path;
  if (!ParseDocumentsProviderUrl(url, &authority, &root_document_id, &tmp_path))
    return nullptr;

  ArcDocumentsProviderRoot* root = Lookup(authority, root_document_id);
  if (!root)
    return nullptr;

  *path = tmp_path;
  return root;
}

ArcDocumentsProviderRoot* ArcDocumentsProviderRootMap::Lookup(
    const std::string& authority,
    const std::string& root_document_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto iter = map_.find(Key(authority, root_document_id));
  if (iter == map_.end())
    return nullptr;
  return iter->second.get();
}

void ArcDocumentsProviderRootMap::Shutdown() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // ArcDocumentsProviderRoot has a reference to another KeyedService
  // (ArcFileSystemOperationRunner), so we need to destruct them on shutdown.
  map_.clear();
}

void ArcDocumentsProviderRootMap::Refresh(
    const ArcDocumentsProviderRootMap::RefreshCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(ryoh): Use BindOnce when it becomes possible.
  runner_->GetRoots(base::Bind(&ArcDocumentsProviderRootMap::OnGetRoots,
                               weak_ptr_factory_.GetWeakPtr(), callback));
}

void ArcDocumentsProviderRootMap::OnGetRoots(
    const ArcDocumentsProviderRootMap::RefreshCallback& callback,
    base::Optional<std::vector<mojom::RootPtr>> rootsFromMojo) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::vector<ArcDocumentsProviderRoot*> roots;
  std::map<Key, std::unique_ptr<ArcDocumentsProviderRoot>> rootsMap;

  if (rootsFromMojo.has_value()) {
    for (const mojom::RootPtr& rootFromMojo : *rootsFromMojo) {
      const std::string& authority = rootFromMojo->authority;
      const std::string& document_id = rootFromMojo->document_id;
      const Key key = Key(authority, document_id);
      auto iter = map_.find(key);
      if (iter != map_.end()) {
        roots.push_back(iter->second.get());
        rootsMap[key] = std::move(iter->second);
        map_.erase(iter);
        continue;
      }
      std::unique_ptr<ArcDocumentsProviderRoot> root =
          base::MakeUnique<ArcDocumentsProviderRoot>(
              runner_, authority, document_id, rootFromMojo->id,
              rootFromMojo->title, rootFromMojo->summary,
              rootFromMojo->icon_data, rootFromMojo->flags);
      roots.push_back(root.get());
      rootsMap[key] = std::move(root);
    }
  }

  map_ = std::move(rootsMap);

  callback.Run(std::move(roots));
}

}  // namespace arc
