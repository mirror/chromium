// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_ENTRY_READER_CT_H_
#define NET_TOOLS_CERT_MAPPER_ENTRY_READER_CT_H_

#include <memory>

#include "base/files/file_path.h"
#include "net/tools/cert_mapper/entry_reader.h"

namespace net {

// Creates an EntryReader that iterates over entries of a dumped CT database (as
// created by dump-ct.sh).
std::unique_ptr<EntryReader> CreateEntryReaderForCertificateTransparencyDb(
    const base::FilePath& path);

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_ENTRY_READER_CT_H_
