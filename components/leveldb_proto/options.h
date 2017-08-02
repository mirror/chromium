// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEVELDB_PROTO_OPTIONS_H_
#define COMPONENTS_LEVELDB_PROTO_OPTIONS_H_

#include "base/files/file_path.h"
#include "third_party/leveldatabase/env_chromium.h"

namespace leveldb_proto {

struct Options {
  Options(const base::FilePath& database_dir,
          size_t write_buffer_size = 0,
          leveldb_env::SharedReadCache shared_cache =
              leveldb_env::SharedReadCache::Default)
      : database_dir(database_dir),
        write_buffer_size(write_buffer_size),
        shared_cache(shared_cache) {}

  base::FilePath database_dir;
  size_t write_buffer_size;
  leveldb_env::SharedReadCache shared_cache;
};

}  // namespace leveldb_proto

#endif  // COMPONENTS_LEVELDB_PROTO_OPTIONS_H_
