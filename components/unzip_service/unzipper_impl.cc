// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/unzip_service/unzipper_impl.h"

#include "third_party/zlib/google/zip.h"
#include "third_party/zlib/google/zip_reader.h"

namespace unzip {

namespace {

// A writer delegate that reports errors instead of writing.
class DudWriterDelegate : public zip::WriterDelegate {
 public:
  DudWriterDelegate() {}
  ~DudWriterDelegate() override {}

  // WriterDelegate methods:
  bool PrepareOutput() override { return false; }
  bool WriteBytes(const char* data, int num_bytes) override { return false; }
  void SetTimeModified(const base::Time& time) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DudWriterDelegate);
};

std::unique_ptr<zip::WriterDelegate> MakeFileWriterDelegate(
    filesystem::mojom::DirectoryPtr output_dir,
    const base::FilePath& path) {
  // TODO: guarantee that parents exist?
  auto file = base::MakeUnique<base::File>();
  if (output_dir->OpenFileHandle(
          path.value(),
          base::File::Flags::FLAG_CREATE | base::File::Flags::FLAG_WRITE,
          nullptr, file.get()))
    return base::MakeUnique<DudWriterDelegate>();
  VLOG(0) << "REACHD";
  return base::MakeUnique<zip::FileWriterDelegate>(std::move(file));
}

bool FilterNoFiles(const base::FilePath& unused) {
  return true;
}

}  // namespace

UnzipperImpl::UnzipperImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

UnzipperImpl::~UnzipperImpl() = default;

void UnzipperImpl::Unzip(base::File zip_file,
                         filesystem::mojom::DirectoryPtr output_dir,
                         UnzipCallback callback) {
  DCHECK(zip_file.IsValid());

  std::move(callback).Run(zip::UnzipWithFilterAndWriters(
      zip_file,
      base::BindRepeating(&MakeFileWriterDelegate,
                          base::Passed(std::move(output_dir))),
      base::Bind(&FilterNoFiles), false));
}

}  // namespace unzip
