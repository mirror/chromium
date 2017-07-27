// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/entry_reader_pem.h"

#include <iostream>
#include <memory>

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
    base::FileEnumerator file_enumerator(dir_path, false, base::FileEnumerator::FILES,
                              FILE_PATH_LITERAL("*.pem"));

    for (base::FilePath path = file_enumerator.Next(); !path.empty();
         path = file_enumerator.Next()) {
      file_paths_.push_back(path);
    }

    if (file_paths_.empty()) {
      std::cerr << "ERROR: No *.pem files within: " << dir_path.value() << "\n";
      return false;
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
    entry.type = Entry::Type::kLeafCert;

    size_t num_certs = 0;
    while (pem_tokenizer.GetNext()) {
      num_certs++;

      der::Input cert_data = SaveCertData(pem_tokenizer.data());

      if (num_certs == 1) {
        entry.cert = cert_data;
      } else {
        entry.extra_certs.push_back(cert_data);
      }
    }

    if (num_certs == 0) {
      std::cerr << "ERROR: No CERTIFICATE blocks found in: " << path.value() << "\n";
      return true;  // Keep going.
    }

    entries->push_back(entry);
    return true;
  }

  der::Input SaveCertData(const std::string& data) {
    // TODO(eroman): Eeew, leaking the data! But the code assumes data
    // pointers remain alive right now....
    std::string* leaked_cert_data = new std::string(data);
    return der::Input(leaked_cert_data);
  }

  std::vector<base::FilePath> file_paths_;

  base::Lock lock_;
  size_t next_path_index_ = 0;
  bool stopped_ = false;
};

}  // namespace

std::unique_ptr<EntryReader> CreateEntryReaderForPemFiles(
    const base::FilePath& path) {
  if (!base::DirectoryExists(path)) {
    std::cerr
        << "ERROR: the input must be a directory (containing .pem files)\n";
    return nullptr;
  }

  std::unique_ptr<PemFilesReader> result(new PemFilesReader());
  if (!result->Init(path))
    return nullptr;

  return result;
}

}  // namespace net
