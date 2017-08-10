// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

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
  ~ArcDocumentsProviderRootMap() override;

  // Returns an instance for the primary profile.
  // If ARC++ is not allowed for the profile, nullptr is returned.
  // TODO(nya): Use GetForProfile() everywhere and get rid of use of this
  // function.
  static ArcDocumentsProviderRootMap* GetForPrimaryProfile();

  // Returns an instance for the given profile.
  // If ARC++ is not allowed for the profile, nullptr is returned.
  static ArcDocumentsProviderRootMap* GetForProfile(Profile* profile);

  // Looks up a root corresponding to |url|.
  // |path| is set to the remaining path part of |url|.
  // Returns nullptr if |url| is invalid or no corresponding root is registered.
  ArcDocumentsProviderRoot* ParseAndLookup(const storage::FileSystemURL& url,
                                           base::FilePath* path) const;

 private:
  friend class ArcDocumentsProviderRootMapFactory;

  explicit ArcDocumentsProviderRootMap(Profile* profile);

  // Key is (authority, root_document_id).
  using Key = std::pair<std::string, std::string>;
  std::map<Key, std::unique_ptr<ArcDocumentsProviderRoot>> map_;

  DISALLOW_COPY_AND_ASSIGN(ArcDocumentsProviderRootMap);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_H_
