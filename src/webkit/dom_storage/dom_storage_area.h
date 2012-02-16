// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_DOM_STORAGE_DOM_STORAGE_AREA_H_
#define WEBKIT_DOM_STORAGE_DOM_STORAGE_AREA_H_
#pragma once

#include "base/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/nullable_string16.h"
#include "base/string16.h"
#include "googleurl/src/gurl.h"
#include "webkit/dom_storage/dom_storage_task_runner.h"

class FilePath;
class GURL;

namespace dom_storage {

class DomStorageMap;

// Container for a per-origin Map of key/value pairs potentially
// backed by storage on disk and lazily commits changes to disk.
// See class comments for DomStorageContext for a larger overview.
class DomStorageArea
    : public base::RefCountedThreadSafe<DomStorageArea> {

 public:
  DomStorageArea(int64 namespace_id,
                 const GURL& origin,
                 const FilePath& directory,
                 DomStorageTaskRunner* task_runner);

  const GURL& origin() const { return origin_; }
  int64 namespace_id() const { return namespace_id_; }

  unsigned Length();
  NullableString16 Key(unsigned index);
  NullableString16 GetItem(const string16& key);
  bool SetItem(const string16& key, const string16& value,
               NullableString16* old_value);
  bool RemoveItem(const string16& key, string16* old_value);
  bool Clear();

  DomStorageArea* ShallowCopy(int64 destination_namespace_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(DomStorageAreaTest, DomStorageAreaBasics);
  friend class base::RefCountedThreadSafe<DomStorageArea>;

  ~DomStorageArea();

  int64 namespace_id_;
  GURL origin_;
  FilePath directory_;
  scoped_refptr<DomStorageTaskRunner> task_runner_;
  scoped_refptr<DomStorageMap> map_;
  // TODO(benm): integrate with DomStorageDatabase to read from
  // and lazily write to disk.
};

}  // namespace dom_storage

#endif  // WEBKIT_DOM_STORAGE_DOM_STORAGE_AREA_H_
