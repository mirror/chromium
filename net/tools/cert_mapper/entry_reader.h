// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_ENTRY_READER_H_
#define NET_TOOLS_CERT_MAPPER_ENTRY_READER_H_

#include <memory>

#include "base/files/file_path.h"
#include "net/tools/cert_mapper/entry_reader.h"

namespace net {

class Entry;

// Interface for iterating over a corpus of "entries" (certificates, or
// certificate chains). Implementations must be thread-safe, as this is called
// from worker threads to get more data.
class EntryReader {
 public:
  virtual ~EntryReader();

  // Reads up to |max_entries| and fills them in |entries|.
  //
  // Note that this may be called in parallel from multiple threads.
  virtual bool Read(std::vector<Entry>* entries, size_t max_entries) = 0;

  // Returns false if the EntryReader has no more entries.
  //
  // Fills |progress| with a value in range from 0 to 1.
  // Fills |num_entries_read| with the number of entries that have been read.
  virtual bool GetProgress(double* progress, size_t* num_entries_read) = 0;

  // Causes subsequent calls to Read() to return false (even if more data is
  // left).
  virtual void Stop() = 0;
};

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_ENTRY_READER_H_
