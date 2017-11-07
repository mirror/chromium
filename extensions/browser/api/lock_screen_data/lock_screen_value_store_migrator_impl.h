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

// Lock screen value store migrator implementation.
// Migrates data items from deprecated (shared) lock screem data item storage
// to the per-user lock screen data item storage.
class LockScreenValueStoreMigratorImpl : public LockScreenValueStoreMigrator {
 public:
  LockScreenValueStoreMigratorImpl(content::BrowserContext* context,
                                   ValueStoreCache* source_store,
                                   ValueStoreCache* target_store,
                                   base::SequencedTaskRunner* task_runner,
                                   const std::string& crypto_key);
  ~LockScreenValueStoreMigratorImpl() override;

  // LockScreenValueStorageMigrator:
  void Run(const std::set<std::string>& extensions_to_migrate,
           const ExtensionMigratedCallback& callback) override;
  bool MigratingExtensionData(const std::string& extension_id) const override;
  void ClearDataForExtension(const std::string& extension_id,
                             const base::Closure& callback) override;

 private:
  // Per-extension migration status data.
  struct MigrationData {
    MigrationData();
    ~MigrationData();

    // List of data items that have yet to be migrated.
    std::vector<std::unique_ptr<DataItem>> pending;

    // Migration source representation of the data item that is currently being
    // migrated.
    std::unique_ptr<DataItem> current_source;

    // Migration target representation of the data item that is currently being
    // migrated.
    std::unique_ptr<DataItem> current_target;
  };

  // Starts migration for an extension - gets the list of all items registered
  // for the extnesion in the source storage.
  void StartMigrationForExtension(const std::string& extension_id);

  // Called when the set of items registered for an extension in the source
  // storage has been retrieved. It initializes the extension's migration data
  // and starts data item migration.
  void OnGotItemsForExtension(const std::string& extension_id,
                              OperationResult result,
                              std::unique_ptr<base::DictionaryValue> items);

  // Starts migration for then next data item in the extension's migration
  // queue - it reads the item contents from the source storage.
  void MigrateNextForExtension(const std::string& extension_id);

  // Called when the contents of the currently migrating item has been read from
  // the source storage - it registers the item in the target storage.
  void OnCurrentItemRead(const std::string& extension_id,
                         OperationResult result,
                         std::unique_ptr<std::vector<char>> data);

  // Called when the currently migrating item has been registered in the target
  // storage - it writes the item contents to the target storage.
  void OnTargetItemRegistered(const std::string& extension_id,
                              std::unique_ptr<std::vector<char>> data,
                              OperationResult result);

  // Called when the contents of the currently migrating dat aitem has been
  // written to the target storage - it deletes the item from the source
  // storage.
  void OnTargetItemWritten(const std::string& extension_id,
                           OperationResult result);

  // Called when the currently migrating item has been deleted from the source
  // storage - i.e. when the migration for the item finished. It starts
  // migration for the next item in the extension's queue.
  void OnCurrentItemMigrated(const std::string& extension_id,
                             OperationResult result);

  // Deletes all extension's data items from the migration source value store.
  void DeleteItemsFromSourceStore(const std::string& extension_id,
                                  const base::Closure& callback);

  // Runs the extension data deletion callback - |callback|. The main purpose
  // is to wrap |callback| to endure it's not called after |this| has been
  // deleted.
  void RunClearDataForExtensionCallback(const base::Closure& callback);

  // Clears data for the extension from |extensions_to_migrate_| and
  // |migration_items_|.
  void ClearMigrationData(const std::string& extension_id);

  content::BrowserContext* context_;

  ExtensionMigratedCallback callback_;

  // Set of extensions whose data is being migrated.
  std::set<std::string> extensions_to_migrate_;

  ValueStoreCache* source_store_cache_;
  ValueStoreCache* target_store_cache_;
  base::SequencedTaskRunner* task_runner_;

  // Crypto key used to encrypt/decrypt data items in the storage.
  const std::string crypto_key_;

  // Maps extension ids to the extension's migration status.
  std::unordered_map<std::string, MigrationData> migration_items_;

  base::WeakPtrFactory<LockScreenValueStoreMigratorImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenValueStoreMigratorImpl);
};

}  // namespace lock_screen_data
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_LOCK_SCREEN_DATA_LOCK_SCREEN_VALUE_STORE_MIGRATOR_IMPL_H_
