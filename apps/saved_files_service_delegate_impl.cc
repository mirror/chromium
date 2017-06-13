// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/saved_files_service_delegate_impl.h"

#include "apps/saved_files_service.h"
#include "content/public/browser/browser_context.h"

namespace apps {

SavedFilesServiceDelegateImpl::SavedFilesServiceDelegateImpl(
    content::BrowserContext* browser_context) {
  saved_files_service_ = SavedFilesService::Get(browser_context);
}

SavedFilesServiceDelegateImpl::~SavedFilesServiceDelegateImpl() {}

bool SavedFilesServiceDelegateImpl::IsRegistered(
    const std::string& extension_id,
    const std::string& id) {
  return saved_files_service_->IsRegistered(extension_id, id);
}

void SavedFilesServiceDelegateImpl::RegisterFileEntry(
    const std::string& extension_id,
    const File& file) {
  saved_files_service_->RegisterFileEntry(extension_id, file.id, file.path,
                                          file.is_directory);
}

bool SavedFilesServiceDelegateImpl::GetRegisteredFileInfo(
    const std::string& extension_id,
    const std::string& id,
    File* file) {
  const SavedFileEntry* entry =
      saved_files_service_->GetFileEntry(extension_id, id);
  if (!entry)
    return false;
  file->id = entry->id;
  file->path = entry->path;
  file->is_directory = entry->is_directory;
  return true;
}

void SavedFilesServiceDelegateImpl::EnqueueFileEntry(
    const std::string& extension_id,
    const std::string& id) {
  saved_files_service_->EnqueueFileEntry(extension_id, id);
}

}  // namespace apps
