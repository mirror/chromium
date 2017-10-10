// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/chrome_file_util/zip_file_creator_impl.h"

#include <memory>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "components/filesystem/public/interfaces/types.mojom-shared.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/zlib/google/zip.h"

namespace chrome {

namespace {

class MojoFileAccessor : public zip::FileAccessor {
 public:
  MojoFileAccessor(const base::FilePath& source_dir,
                   filesystem::mojom::DirectoryPtr source_dir_mojo)
      : source_dir_(source_dir), source_dir_mojo_(std::move(source_dir_mojo)) {}
  ~MojoFileAccessor() override = default;

 private:
  base::File OpenFileForReading(const base::FilePath& path) override {
    DCHECK(source_dir_.IsParent(path));
    base::FilePath relative_path = GetPathRelativeToSourceDir(path);
    uint32_t open_flags =
        filesystem::mojom::kFlagRead | filesystem::mojom::kFlagOpen;
    filesystem::mojom::FileError error;
    base::File file;
    source_dir_mojo_->OpenFileHandle(relative_path.value(), open_flags, &error,
                                     &file);
    if (error != filesystem::mojom::FileError::OK)
      LOG(ERROR) << "Failed to open file " << relative_path.value();
    return file;
  }

  bool DirectoryExists(const base::FilePath& path) override {
    if (source_dir_ == path)
      return true;
    DCHECK(source_dir_ == path || source_dir_.IsParent(path));
    filesystem::mojom::FileError error;
    filesystem::mojom::FileInformationPtr file_info;
    source_dir_mojo_->StatFile(GetPathRelativeToSourceDir(path).value(), &error,
                               &file_info);
    if (error != filesystem::mojom::FileError::OK || !file_info)
      return false;  // No such file.
    return file_info->type == filesystem::mojom::FsFileType::DIRECTORY;
  }

  std::vector<DirectoryEntry> ListDirectoryContent(
      const base::FilePath& dir_path) override {
    DCHECK(source_dir_.IsParent(dir_path));

    std::vector<DirectoryEntry> results;
    base::FilePath relative_path = GetPathRelativeToSourceDir(dir_path);
    filesystem::mojom::DirectoryPtr dir_mojo;
    filesystem::mojom::FileError error;
    source_dir_mojo_->OpenDirectory(
        relative_path.value(), mojo::MakeRequest(&dir_mojo),
        filesystem::mojom::kFlagRead | filesystem::mojom::kFlagOpen, &error);
    if (error != filesystem::mojom::FileError::OK) {
      LOG(ERROR) << "Failed to open " << relative_path.value() << " error "
                 << error;
      return results;
    }

    base::Optional<std::vector<filesystem::mojom::DirectoryEntryPtr>>
        directory_contents;
    dir_mojo->Read(&error, &directory_contents);
    if (error != filesystem::mojom::FileError::OK) {
      LOG(ERROR) << "Failed to list content of " << dir_path.value()
                 << " error " << error;
    }
    if (directory_contents) {
      results.reserve(directory_contents->size());
      for (const filesystem::mojom::DirectoryEntryPtr& entry :
           *directory_contents) {
        base::FilePath path(entry->name);
        bool is_directory =
            entry->type == filesystem::mojom::FsFileType::DIRECTORY;
        results.push_back(std::make_pair(path, is_directory));
      }
    }
    return results;
  }

  base::Time GetLastModifiedTime(const base::FilePath& path) override {
    DCHECK(source_dir_.IsParent(path));
    base::FilePath relative_path = GetPathRelativeToSourceDir(path);
    filesystem::mojom::FileError error;
    filesystem::mojom::FileInformationPtr file_info;
    source_dir_mojo_->StatFile(relative_path.value(), &error, &file_info);
    if (error != filesystem::mojom::FileError::OK) {
      LOG(ERROR) << "Failed to get last modified time of "
                 << relative_path.value() << " error " << error;
      return base::Time();
    }
    return base::Time::FromDoubleT(file_info->mtime);
  }

  base::FilePath GetPathRelativeToSourceDir(const base::FilePath& path) {
    if (!path.IsAbsolute())
      return path;
    base::FilePath relative_path;
    bool result = source_dir_.AppendRelativePath(path, &relative_path);
    DCHECK(result);
    return relative_path;
  }

  base::FilePath source_dir_;
  filesystem::mojom::DirectoryPtr source_dir_mojo_;

  DISALLOW_COPY_AND_ASSIGN(MojoFileAccessor);
};

}  // namespace

ZipFileCreatorImpl::ZipFileCreatorImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

ZipFileCreatorImpl::~ZipFileCreatorImpl() = default;

void ZipFileCreatorImpl::CreateZipFile(
    filesystem::mojom::DirectoryPtr source_dir_mojo,
    const base::FilePath& source_dir,
    const std::vector<base::FilePath>& source_relative_paths,
    base::File zip_file,
    CreateZipFileCallback callback) {
  DCHECK(zip_file.IsValid());

  for (const auto& path : source_relative_paths) {
    if (path.IsAbsolute() || path.ReferencesParent()) {
      std::move(callback).Run(false);
      return;
    }
  }

  zip::ZipParams zip_params(source_dir, zip_file.GetPlatformFile());
  zip_params.set_files_to_zip(source_relative_paths);
  zip_params.set_file_accessor(std::make_unique<MojoFileAccessor>(
      source_dir, std::move(source_dir_mojo)));
  std::move(callback).Run(zip::Zip(zip_params));
}

}  // namespace chrome