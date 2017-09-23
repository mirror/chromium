// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LEVELDB_LEVELDB_STRUCT_TRAITS_H_
#define COMPONENTS_LEVELDB_LEVELDB_STRUCT_TRAITS_H_

#include "components/leveldb/public/interfaces/leveldb.mojom.h"

namespace mojo {

template <>
struct StructTraits<leveldb::mojom::OpenOptionsDataView,
                    leveldb_chrome::Options> {
  static bool create_if_missing(const leveldb_chrome::Options& options);
  static bool error_if_exists(const leveldb_chrome::Options& options);
  static bool paranoid_checks(const leveldb_chrome::Options& options);
  static uint64_t write_buffer_size(const leveldb_chrome::Options& options);
  static int32_t max_open_files(const leveldb_chrome::Options& options);
  static ::leveldb::mojom::SharedReadCache shared_block_read_cache(
      const leveldb_chrome::Options& options);
  static bool Read(::leveldb::mojom::OpenOptionsDataView data,
                   leveldb_chrome::Options* out);
};

}  // namespace mojo

#endif  // COMPONENTS_LEVELDB_LEVELDB_STRUCT_TRAITS_H_
