// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_TEST_EXTERNAL_CACHE_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_TEST_EXTERNAL_CACHE_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/chromeos/extensions/external_cache.h"

namespace chromeos {

class ExternalCacheDelegate;

class TestExternalCache : public ExternalCache {
 public:
  TestExternalCache(ExternalCacheDelegate* delegate,
                    bool always_check_for_updates);
  ~TestExternalCache() override;

  // ExternalCache:
  const base::DictionaryValue* GetCachedExtensions() override;
  void Shutdown(const base::Closure& callback) override;
  void UpdateExtensionsList(
      std::unique_ptr<base::DictionaryValue> prefs) override;
  void OnDamagedFileDetected(const base::FilePath& path) override;
  void RemoveExtensions(const std::vector<std::string>& ids) override;
  bool GetExtension(const std::string& id,
                    base::FilePath* file_path,
                    std::string* version) override;
  bool ExtensionFetchPending(const std::string& id) override;
  void PutExternalExtension(
      const std::string& id,
      const base::FilePath& crx_file_path,
      const std::string& version,
      const PutExternalExtensionCallback& callback) override;

  bool SimulateExtensionDownloadFinished(const std::string& id,
                                         const std::string& crx_path,
                                         const std::string& version);
  bool SimulateExtensionDownloadFailed(const std::string& id);

  const std::set<std::string>& pending_downloads() const {
    return pending_downloads_;
  }

 private:
  struct CacheEntry {
    std::string path;
    std::string version;
  };

  void UpdateCache();
  void AddEntryToCache(const std::string& id,
                       const std::string& crx_path,
                       const std::string& version);

  ExternalCacheDelegate* delegate_;
  bool always_check_for_updates_;

  std::unique_ptr<base::DictionaryValue> configured_extensions_;
  base::DictionaryValue cached_extensions_;

  std::set<std::string> pending_downloads_;
  std::map<std::string, CacheEntry> cache_;

  DISALLOW_COPY_AND_ASSIGN(TestExternalCache);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_TEST_EXTERNAL_CACHE_H_
