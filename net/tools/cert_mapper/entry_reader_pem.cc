// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/entry_reader_pem.h"

#include <iostream>
#include <memory>
#include <unordered_set>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/synchronization/lock.h"
#include "net/cert/pem_tokenizer.h"
#include "net/tools/cert_mapper/entry.h"
#include "net/tools/cert_mapper/entry_reader.h"

namespace net {

namespace {

class PemFilesReader : public EntryReader {
 public:
  bool Init(const base::FilePath& dir_path) {
    // Find all the files (recursively).
    base::FileEnumerator file_enumerator(dir_path, true,
                                         base::FileEnumerator::FILES);

    for (base::FilePath path = file_enumerator.Next(); !path.empty();
         path = file_enumerator.Next()) {
      file_paths_.push_back(path);
    }

    if (file_paths_.empty()) {
      std::cerr << "ERROR: No files within: " << dir_path.value() << "\n";
      return false;
    } else {
      std::cerr << "Identified " << file_paths_.size()
                << " files inside of " << dir_path.value() << "\n";
    }

    return true;
  }

  bool Read(std::vector<Entry>* entries, size_t max_entries) override {
    entries->clear();

    base::AutoLock l(lock_);

    if (stopped_)
      return false;

    for (size_t i = 0; i < max_entries; ++i) {
      if (!ReadNextFile(entries))
        break;
    }

    return !entries->empty();
  }

  bool GetProgress(double* progress, size_t* num_entries_read) override {
    base::AutoLock l(lock_);

    *progress = static_cast<double>(next_path_index_) /
                static_cast<double>(file_paths_.size());
    *num_entries_read = next_path_index_;

    return !stopped_ && next_path_index_ < file_paths_.size();
  }

  void Stop() override {
    base::AutoLock l(lock_);
    stopped_ = true;
  }

 private:
  bool ReadNextFile(std::vector<Entry>* entries) {
    if (next_path_index_ >= file_paths_.size())
      return false;

    const base::FilePath& path = file_paths_[next_path_index_++];

    std::string file_data;
    if (!base::ReadFileToString(path, &file_data)) {
      std::cerr << "ERROR: Failed reading: " << path.value() << "\n";
      return true;  // Keep going.
    }

    std::vector<std::string> pem_headers = {"CERTIFICATE"};
    PEMTokenizer pem_tokenizer(file_data, pem_headers);

    Entry entry;
    entry.type = EntryType::kTlsCertificateChain;

    while (pem_tokenizer.GetNext()) {
      const std::string block_data = pem_tokenizer.data();
      entry.certs.push_back(base::MakeRefCounted<CertBytes>(block_data.data(),
                                                            block_data.size()));
    }

    if (entry.certs.empty()) {
      std::cerr << "ERROR: No CERTIFICATE blocks found in: " << path.value()
                << "\n";
      return true;  // Keep going.
    }

    // Emit the intermediates that haven't been seen before.
    for (size_t i = 1; i < entry.certs.size(); ++i) {
      const auto& cert = entry.certs[i];
      bool is_new = seen_intermediates_.insert(cert->AsStringPiece()).second;
      if (is_new) {
        // Keep the data alive (since set uses StringPiece).
        seen_intermediates_storage_.push_back(cert);
        Entry intermediate_entry;
        intermediate_entry.type = EntryType::kUniqueIntermediateCertificate;
        intermediate_entry.certs.push_back(cert);
        entries->push_back(std::move(intermediate_entry));
      }
    }

    entries->push_back(std::move(entry));
    return true;
  }

  std::vector<base::FilePath> file_paths_;

  base::Lock lock_;
  size_t next_path_index_ = 0;
  bool stopped_ = false;
  std::unordered_set<base::StringPiece, base::StringPieceHash>
      seen_intermediates_;
  std::vector<scoped_refptr<CertBytes>> seen_intermediates_storage_;
};

}  // namespace

std::unique_ptr<EntryReader> CreateEntryReaderForPemFiles(
    const base::FilePath& path) {
  if (!base::DirectoryExists(path)) {
    std::cerr
        << "ERROR: the input must be a directory (containing PEM files)\n";
    return nullptr;
  }

  std::unique_ptr<PemFilesReader> result(new PemFilesReader());
  if (!result->Init(path))
    return nullptr;

  return result;
}

}  // namespace net
