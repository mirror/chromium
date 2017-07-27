// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/entry_reader.h"

#include <iostream>
#include <memory>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "net/cert/pem_tokenizer.h"
#include "net/tools/cert_mapper/entry.h"

namespace net {

namespace {

class BytesReader {
 public:
  BytesReader() {}

  BytesReader(const uint8_t* data, size_t size) { Assign(data, size); }

  void Assign(const uint8_t* data, size_t size) {
    data_ = data;
    size_ = size;
  }

  bool ReadBytes(size_t count, const uint8_t** out) WARN_UNUSED_RESULT {
    if (position_ + count > size_)
      return false;  // Not enough data left.
    *out = (data_ + position_);
    position_ += count;
    return true;
  }

  bool ReadBigEndianUint32(uint32_t* out) WARN_UNUSED_RESULT {
    const uint8_t* x;
    if (!ReadBytes(4, &x))
      return false;
    *out = x[0] << 24 | x[1] << 16 | x[2] << 8 | x[3];
    return true;
  }

  bool ReadBigEndianUint16(uint16_t* out) WARN_UNUSED_RESULT {
    const uint8_t* x;
    if (!ReadBytes(2, &x))
      return false;
    *out = x[0] << 8 | x[1];
    return true;
  }

  bool eof() const { return position_ == size_; }

  size_t position() const { return position_; }

 private:
  const uint8_t* data_ = nullptr;
  size_t size_ = 0;
  size_t position_ = 0;
};

bool ReadCertificateBytes(BytesReader* reader, der::Input* cert) {
  uint32_t cert_len;
  if (!reader->ReadBigEndianUint32(&cert_len)) {
    if (!reader->eof()) {
      std::cerr << "Failed reading certificate length (4 byte big-endian "
                   "number) at position="
                << reader->position() << "\n";
    }
    return false;
  }

  const uint8_t* cert_bytes;
  if (!reader->ReadBytes(cert_len, &cert_bytes)) {
    std::cerr << "Failed reading reading certificate of length=" << cert_len
              << " from position=" << reader->position() << "\n";
    return false;
  }

  *cert = net::der::Input(cert_bytes, cert_len);
  return true;
}

std::unique_ptr<base::MemoryMappedFile> LoadFile(const base::FilePath& path) {
  std::unique_ptr<base::MemoryMappedFile> file(new base::MemoryMappedFile());

  if (!file->Initialize(path) || !file->IsValid()) {
    std::cerr << "ERROR: Couldn't initialize memory mapped file: "
              << path.value() << "\n";
    return nullptr;
  }

  return file;
}

class CtDumpEntryReader : public EntryReader {
 public:
  bool Init(const base::FilePath& entries_path,
            const base::FilePath& extra_certs_path) {
    // Read in all of the extra certs.
    if (!base::ReadFileToString(extra_certs_path, &extra_certs_data_)) {
      std::cerr << "ERROR: Couldn't read extra certs file: "
                << extra_certs_path.value() << "\n";
      return false;
    }

    BytesReader extra_certs_reader(
        reinterpret_cast<const uint8_t*>(extra_certs_data_.data()),
        extra_certs_data_.size());

    der::Input cert;
    while (ReadCertificateBytes(&extra_certs_reader, &cert)) {
      extra_certs_.push_back(cert);
    }

    entries_file_ = LoadFile(entries_path);
    if (!entries_file_)
      return false;

    entries_reader_.Assign(entries_file_->data(), entries_file_->length());
    return true;
  }

  bool ReadAndAppend(std::vector<Entry>* out) {
    der::Input cert;
    if (!ReadCertificateBytes(&entries_reader_, &cert))
      return false;

    uint16_t num_extra_certs;
    if (!entries_reader_.ReadBigEndianUint16(&num_extra_certs)) {
      std::cerr << "Failed reading number of extra certs at position="
                << entries_reader_.position() << "\n";
      return false;
    }

    std::vector<der::Input> extra_certs;
    extra_certs.reserve(num_extra_certs);

    for (size_t i = 0; i < num_extra_certs; ++i) {
      uint16_t extra_cert_index;
      if (!entries_reader_.ReadBigEndianUint16(&extra_cert_index)) {
        std::cerr << "Failed reading extra_cert at position="
                  << entries_reader_.position() << "\n";
        return false;
      }

      if (extra_cert_index >= extra_certs_.size()) {
        std::cerr << "Invalid extra_cert_index=" << extra_cert_index
                  << " at position=" << entries_reader_.position() << "\n";
        return false;
      }

      extra_certs.push_back(extra_certs_[extra_cert_index]);
    }

    out->emplace_back(cert);
    std::swap(out->back().extra_certs, extra_certs);
    return true;
  }

  bool Read(std::vector<Entry>* entries, size_t max_entries) override {
    entries->clear();

    while (num_consumed_extra_certs_ < extra_certs_.size() &&
           entries->size() < max_entries) {
      entries->emplace_back(Entry::Type::kExtraCert,
                            extra_certs_[num_consumed_extra_certs_++]);
    }

    while (entries->size() < max_entries && ReadAndAppend(entries))
      ;

    return !entries->empty();
  }

  double GetProgress() const override {
    return static_cast<double>(entries_reader_.position()) /
           static_cast<double>(entries_file_->length());
  }

 private:
  std::unique_ptr<base::MemoryMappedFile> entries_file_;
  BytesReader entries_reader_;

  std::string extra_certs_data_;
  std::vector<der::Input> extra_certs_;
  size_t num_consumed_extra_certs_ = 0;
};

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

    for (size_t i = 0; i < max_entries; ++i) {
      if (!ReadNextFile(entries))
        break;
    }

    return !entries->empty();
  }

  double GetProgress() const override {
    return static_cast<double>(next_path_index_) /
           static_cast<double>(file_paths_.size());
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

  size_t next_path_index_ = 0;
  std::vector<base::FilePath> file_paths_;
};

}  // namespace

std::unique_ptr<EntryReader> CreateEntryReaderForCertificateTransparencyDb(
    const base::FilePath& path) {
  if (!base::DirectoryExists(path)) {
    std::cerr
        << "ERROR: the input must be a directory (containing CT db dump)\n";
    return nullptr;
  }

  std::unique_ptr<CtDumpEntryReader> result(new CtDumpEntryReader());
  if (!result->Init(path.AppendASCII("entries.bin"),
                    path.AppendASCII("extra_certs.bin"))) {
    return nullptr;
  }

  return std::move(result);
}

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
