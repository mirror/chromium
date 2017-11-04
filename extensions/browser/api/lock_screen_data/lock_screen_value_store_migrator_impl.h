// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_IMPL_H_
#define EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_IMPL_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "extensions/browser/api/lock_screen_data/lock_screen_value_store_migrator.h"
#include "extensions/browser/api/lock_screen_data/operation_result.h"

namespace base {
class DictionaryValue;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}

namespace extensions {
class ValueStoreCache;

namespace lock_screen_data {

class DataItem;

class LockScreenValueStoreMigratorImpl : public LockScreenValueStoreMigrator {
 public:
  LockScreenValueStoreMigratorImpl(content::BrowserContext* context,
                                   ValueStoreCache* source_store,
                                   ValueStoreCache* target_store,
                                   base::SequencedTaskRunner* task_runner,
                                   const std::string& crypto_key);
  ~LockScreenValueStoreMigratorImpl() override;

  void Run(const std::set<std::string>& extensions_to_migrate,
           const ExtensionMigratedCallback& callback) override;
  void ClearDataForExtension(const std::string& extension_id,
                             const base::Closure& callback) override;
  bool MigratingExtensionData(const std::string& extension_id) const override;

 private:
  struct MigrationData {
    MigrationData();
    ~MigrationData();

    std::vector<std::unique_ptr<DataItem>> pending;
    std::unique_ptr<DataItem> current_source;
    std::unique_ptr<DataItem> current_target;
  };

  void StartMigrationForExtension(const std::string& extension_id);
  void OnGotItemsForExtension(const std::string& extension_id,
                              OperationResult result,
                              std::unique_ptr<base::DictionaryValue> items);
  void MigrateNextForExtension(const std::string& extension_id);
  void OnCurrentItemRead(const std::string& extension_id,
                         OperationResult result,
                         std::unique_ptr<std::vector<char>> data);
  void OnTargetItemRegistered(const std::string& extension_id,
                              std::unique_ptr<std::vector<char>> data,
                              OperationResult result);
  void OnTargetItemWritten(const std::string& extension_id,
                           OperationResult result);
  void OnCurrentItemMigrated(const std::string& extension_id,
                             OperationResult result);

  void DeleteAllDeprecatedItems(const std::string& extension_id,
                                const base::Closure& callback);
  void RunClearDataForExtensionCallback(const base::Closure& callback);

  void ClearMigrationData(const std::string& extension_id);

  content::BrowserContext* context_;

  ExtensionMigratedCallback callback_;

  std::set<std::string> extensions_to_migrate_;

  ValueStoreCache* source_store_cache_;
  ValueStoreCache* target_store_cache_;
  base::SequencedTaskRunner* task_runner_;

  const std::string crypto_key_;

  std::unordered_map<std::string, MigrationData> migration_items_;

  base::WeakPtrFactory<LockScreenValueStoreMigratorImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenValueStoreMigratorImpl);
};

}  // namespace lock_screen_data
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_IMPL_H_
