// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_FILE_SYSTEM_SAVED_FILES_SERVICE_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_FILE_SYSTEM_SAVED_FILES_SERVICE_DELEGATE_H_

#include <string>

#include "base/files/file_path.h"

namespace extensions {
namespace file_system_api {

// TODO(michaelpg): These methods are abstracted into a delegate only because
// any real implementation will be dependent on apps::SavedFilesService, which
// the extensions module must not depend on. Ideally, the chrome.fileSystem API
// itself would be implemented in //apps and we wouldn't have this problem.
class SavedFilesServiceDelegate {
 public:
  // Basic information about a registered file.
  struct File {
    std::string id;
    base::FilePath path;
    bool is_directory;
  };

  virtual ~SavedFilesServiceDelegate(){};

  // Returns whether the file with the given |id| has been registered.
  virtual bool IsRegistered(const std::string& extension_id,
                            const std::string& id) = 0;

  // Registers a file with the saved files service.
  virtual void RegisterFileEntry(const std::string& extension_id,
                                 const File& file) = 0;

  // If the file is registered, populates |file| with its information and
  // returns true.
  virtual bool GetRegisteredFileInfo(const std::string& extension_id,
                                     const std::string& id,
                                     File* file) = 0;

  // Moves or adds |id| to the back of the queue of files to be retained.
  virtual void EnqueueFileEntry(const std::string& extension_id,
                                const std::string& id) = 0;
};

}  // namespace file_system_api
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_FILE_SYSTEM_SAVED_FILES_SERVICE_DELEGATE_H_
