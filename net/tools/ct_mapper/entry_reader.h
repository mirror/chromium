// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CT_MAPPER_ENTRY_READER_H_
#define NET_TOOLS_CT_MAPPER_ENTRY_READER_H_

#include <memory>

#include "base/files/file_path.h"
#include "net/tools/ct_mapper/entry_reader.h"

namespace net {

class Entry;

class EntryReader {
 public:
  virtual ~EntryReader() {}

  // Reads up to max_entries and fills them in |entries|.
  virtual bool Read(std::vector<Entry>* entries, size_t max_entries) = 0;

  // Returns a number in the range 0-1.
  virtual double GetProgress() const = 0;
};

std::unique_ptr<EntryReader> CreateEntryReaderForCertificateTransparencyDb(
    const base::FilePath& path);

}  // namespace net

#endif  // NET_TOOLS_CT_MAPPER_ENTRY_READER_H_
