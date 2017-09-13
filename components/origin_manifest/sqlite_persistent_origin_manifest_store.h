// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_SQLITE_PERSISTENT_ORIGIN_MANIFEST_STORE_H_
#define COMPONENTS_ORIGIN_MANIFEST_SQLITE_PERSISTENT_ORIGIN_MANIFEST_STORE_H_

#include <list>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace origin_manifest {

class SQLitePersistentOriginManifestStore
    : public base::RefCountedThreadSafe<SQLitePersistentOriginManifestStore> {
 public:
  typedef base::Callback<void(std::vector<mojom::OriginManifestPtr>)>
      LoadedCallback;

  // All blocking database accesses will be performed on
  // |background_task_runner|, while |client_task_runner| is used to invoke
  // callbacks.
  SQLitePersistentOriginManifestStore(
      const base::FilePath& path,
      const scoped_refptr<base::SequencedTaskRunner>& client_task_runner,
      const scoped_refptr<base::SequencedTaskRunner>& background_task_runner);

  void DeleteAllInList(const std::list<std::string>& origins);

  void Close(const base::Closure& callback);

  void Load(const LoadedCallback& loaded_callback);
  void LoadOriginManifestForOrigin(const std::string& origin,
                                   const LoadedCallback& callback);
  void AddOriginManifest(const mojom::OriginManifestPtr& om);
  void DeleteOriginManifest(const mojom::OriginManifestPtr& om);
  /*
    void SetBeforeFlushCallback(base::RepeatingClosure callback) override;
    void Flush(base::OnceClosure callback) override;
  */

 private:
  friend class base::RefCountedThreadSafe<SQLitePersistentOriginManifestStore>;
  ~SQLitePersistentOriginManifestStore();

  class Backend;

  scoped_refptr<Backend> backend_;

  DISALLOW_COPY_AND_ASSIGN(SQLitePersistentOriginManifestStore);
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_SQLITE_PERSISTENT_ORIGIN_MANIFEST_STORE_H_
