// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/entry_reader_ct.h"

#include <iostream>
#include <memory>

#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "base/synchronization/lock.h"
#include "net/tools/cert_mapper/entry.h"
#include "net/tools/cert_mapper/entry_reader.h"

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

scoped_refptr<CertBytes> ReadCertificateBytes(BytesReader* reader) {
  uint32_t cert_len;
  if (!reader->ReadBigEndianUint32(&cert_len)) {
    if (!reader->eof()) {
      std::cerr << "Failed reading certificate length (4 byte big-endian "
                   "number) at position="
                << reader->position() << "\n";
    }
    return nullptr;
  }

  const uint8_t* cert_bytes;
  if (!reader->ReadBytes(cert_len, &cert_bytes)) {
    std::cerr << "Failed reading reading certificate of length=" << cert_len
              << " from position=" << reader->position() << "\n";
    return nullptr;
  }

  return base::MakeRefCounted<CertBytes>(cert_bytes, cert_len);
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
    std::unique_ptr<base::MemoryMappedFile> extra_certs_file =
        LoadFile(extra_certs_path);
    if (!extra_certs_file)
      return false;

    BytesReader extra_certs_reader(
        reinterpret_cast<const uint8_t*>(extra_certs_file->data()),
        extra_certs_file->length());

    while (scoped_refptr<CertBytes> cert =
               ReadCertificateBytes(&extra_certs_reader)) {
      extra_certs_.push_back(std::move(cert));
    }

    entries_file_ = LoadFile(entries_path);
    if (!entries_file_)
      return false;

    entries_reader_.Assign(entries_file_->data(), entries_file_->length());
    return true;
  }

  bool ReadAndAppend(std::vector<Entry>* out) {
    scoped_refptr<CertBytes> cert = ReadCertificateBytes(&entries_reader_);
    if (!cert)
      return false;

    uint16_t num_extra_certs;
    if (!entries_reader_.ReadBigEndianUint16(&num_extra_certs)) {
      std::cerr << "Failed reading number of extra certs at position="
                << entries_reader_.position() << "\n";
      return false;
    }

    Entry entry;
    entry.type = Entry::Type::kLeafCert;
    entry.certs.reserve(num_extra_certs + 1);
    entry.certs.push_back(std::move(cert));

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

      entry.certs.push_back(extra_certs_[extra_cert_index]);
    }

    out->push_back(std::move(entry));
    return true;
  }

  bool Read(std::vector<Entry>* entries, size_t max_entries) override {
    entries->clear();

    base::AutoLock l(lock_);

    if (stopped_)
      return false;

    while (num_consumed_extra_certs_ < extra_certs_.size() &&
           entries->size() < max_entries) {
      Entry entry;
      entry.type = Entry::Type::kExtraCert;
      entry.certs.push_back(extra_certs_[num_consumed_extra_certs_++]);
      entries->push_back(std::move(entry));
    }

    while (entries->size() < max_entries && ReadAndAppend(entries))
      ;

    num_entries_read_ += entries->size();
    return !entries->empty();
  }

  bool GetProgress(double* progress, size_t* num_entries_read) override {
    base::AutoLock l(lock_);

    *progress = static_cast<double>(entries_reader_.position()) /
                static_cast<double>(entries_file_->length());
    *num_entries_read = num_entries_read_;

    return !stopped_ && entries_reader_.position() < entries_file_->length();
  }

  void Stop() override {
    base::AutoLock l(lock_);
    stopped_ = true;
  }

 private:
  std::unique_ptr<base::MemoryMappedFile> entries_file_;
  CertBytesVector extra_certs_;

  base::Lock lock_;
  BytesReader entries_reader_;
  size_t num_consumed_extra_certs_ = 0;
  bool stopped_ = false;
  size_t num_entries_read_ = 0;
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

}  // namespace net
