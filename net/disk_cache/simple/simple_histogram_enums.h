// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DISK_CACHE_SIMPLE_SIMPLE_HISTOGRAM_ENUMS_H_
#define NET_DISK_CACHE_SIMPLE_SIMPLE_HISTOGRAM_ENUMS_H_

namespace disk_cache {

// Used in histograms, please only add entries at the end.
enum SimpleReadResult {
  READ_RESULT_SUCCESS = 0,
  READ_RESULT_INVALID_ARGUMENT = 1,
  READ_RESULT_NONBLOCK_EMPTY_RETURN = 2,
  READ_RESULT_BAD_STATE = 3,
  READ_RESULT_FAST_EMPTY_RETURN = 4,
  READ_RESULT_SYNC_READ_FAILURE = 5,
  READ_RESULT_SYNC_CHECKSUM_FAILURE = 6,
  READ_RESULT_MAX = 7,
};

// Used in histograms, please only add entries at the end.
enum OpenEntryResult {
  OPEN_ENTRY_SUCCESS = 0,
  OPEN_ENTRY_PLATFORM_FILE_ERROR = 1,
  OPEN_ENTRY_CANT_READ_HEADER = 2,
  OPEN_ENTRY_BAD_MAGIC_NUMBER = 3,
  OPEN_ENTRY_BAD_VERSION = 4,
  OPEN_ENTRY_CANT_READ_KEY = 5,
  OPEN_ENTRY_KEY_MISMATCH = 6,
  OPEN_ENTRY_KEY_HASH_MISMATCH = 7,
  OPEN_ENTRY_SPARSE_OPEN_FAILED = 8,
  OPEN_ENTRY_INVALID_FILE_LENGTH = 9,
  OPEN_ENTRY_MAX = 10,
};

// Used in histograms, please only add entries at the end.
enum SyncWriteResult {
  SYNC_WRITE_RESULT_SUCCESS = 0,
  SYNC_WRITE_RESULT_PRETRUNCATE_FAILURE,
  SYNC_WRITE_RESULT_WRITE_FAILURE,
  SYNC_WRITE_RESULT_TRUNCATE_FAILURE,
  SYNC_WRITE_RESULT_LAZY_STREAM_ENTRY_DOOMED,
  SYNC_WRITE_RESULT_LAZY_CREATE_FAILURE,
  SYNC_WRITE_RESULT_LAZY_INITIALIZE_FAILURE,
  SYNC_WRITE_RESULT_MAX,
};

// Used in histograms, please only add entries at the end.
enum CheckEOFResult {
  CHECK_EOF_RESULT_SUCCESS,
  CHECK_EOF_RESULT_READ_FAILURE,
  CHECK_EOF_RESULT_MAGIC_NUMBER_MISMATCH,
  CHECK_EOF_RESULT_CRC_MISMATCH,
  CHECK_EOF_RESULT_KEY_SHA256_MISMATCH,
  CHECK_EOF_RESULT_MAX,
};

// Used in histograms, please only add entries at the end.
enum CloseResult {
  CLOSE_RESULT_SUCCESS,
  CLOSE_RESULT_WRITE_FAILURE,
  CLOSE_RESULT_MAX,
};

// Used in histograms, please only add entries at the end.
enum class KeySHA256Result { NOT_PRESENT, MATCHED, NO_MATCH, MAX };

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_SIMPLE_SIMPLE_HISTOGRAM_ENUMS_H_
