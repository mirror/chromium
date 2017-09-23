// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_TESTING_H_
#define THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_TESTING_H_

#include "third_party/leveldatabase/leveldb_exports.h"
#include "third_party/leveldatabase/src/db/filename.h"

namespace leveldb_chrome {

namespace testing {

// If filename is a leveldb file, store the type of the file in *type.
// The number encoded in the filename is stored in *number.  If the
// filename was successfully parsed, returns true.  Else return false.
LEVELDB_EXPORT bool ParseFileName(const std::string& filename,
                                  uint64_t* number,
                                  leveldb::FileType* type);

}  // namespace testing

}  // namespace leveldb_chrome

#endif  // THIRD_PARTY_LEVELDATABASE_LEVELDB_CHROME_TESTING_H_
