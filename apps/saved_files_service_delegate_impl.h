// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_SAVED_FILES_SERVICE_DELEGATE_IMPL_H_
#define APPS_SAVED_FILES_SERVICE_DELEGATE_IMPL_H_

#include "extensions/browser/api/file_system/saved_files_service_delegate.h"

#include <string>

#include "base/macros.h"

namespace content {
class BrowserContext;
}

namespace apps {

class SavedFilesService;

// Concrete implementation that just forwards calls to the SavedFilesService.
class SavedFilesServiceDelegateImpl
    : public extensions::file_system_api::SavedFilesServiceDelegate {
 public:
  SavedFilesServiceDelegateImpl(content::BrowserContext* browser_context);
  ~SavedFilesServiceDelegateImpl() override;

  // extensions::file_system_api::SavedFileServiceDelegate:
  bool IsRegistered(const std::string& extension_id,
                    const std::string& id) override;
  void RegisterFileEntry(const std::string& extension_id,
                         const File& file) override;
  bool GetRegisteredFileInfo(const std::string& extension_id,
                             const std::string& id,
                             File* file) override;
  void EnqueueFileEntry(const std::string& extension_id,
                        const std::string& id) override;

 private:
  SavedFilesService* saved_files_service_;

  DISALLOW_COPY_AND_ASSIGN(SavedFilesServiceDelegateImpl);
};

}  // namespace apps

#endif  // APPS_SAVED_FILES_SERVICE_DELEGATE_IMPL_H_
