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

// A zip::FileAccessor that talk to a file system through the Mojo
// filesystem::mojom::Directory.
// Note that zip::ZipFileAccessor deals with absolute paths that must be
// converted to relative when calling filesystem::mojom::Directory APIs.
class MojoFileAccessor : public zip::FileAccessor {
 public:
  MojoFileAccessor(const base::FilePath& source_dir,
                   filesystem::mojom::DirectoryPtr source_dir_mojo)
      : source_dir_(source_dir), source_dir_mojo_(std::move(source_dir_mojo)) {}
  ~MojoFileAccessor() override = default;

 private:
  struct FileInfo {
    FileInfo() {}
    FileInfo(bool is_directory, uint64_t last_modified_time)
        : is_directory(is_directory), last_modified_time(last_modified_time) {}
    bool is_directory = false;
    uint64_t last_modified_time = 0;
  };
  FileInfo null_file_info_;

  std::vector<base::File> OpenFilesForReading(
      const std::vector<base::FilePath>& paths) override {
    TRACE_EVENT0("ZipFileCreator", "OpenFilesForReading");
    std::vector<filesystem::mojom::FileOpenDetailsPtr> details(paths.size());
    size_t i = 0;
    for (const auto& path : paths) {
      filesystem::mojom::FileOpenDetailsPtr open_details(
          filesystem::mojom::FileOpenDetails::New());
      open_details->path = GetRelativePath(path).value();
      open_details->open_flags =
          filesystem::mojom::kFlagOpen | filesystem::mojom::kFlagRead;
      details[i++] = std::move(open_details);
    }
    std::vector<filesystem::mojom::FileOpenResultPtr> results;
    if (!source_dir_mojo_->OpenFileHandles(std::move(details), &results)) {
      return std::vector<base::File>();
    }
    std::vector<base::File> files;
    for (const auto& file_open_result : results) {
      files.push_back(std::move(file_open_result->file_handle));
    }
    return files;
  }

  bool DirectoryExists(const base::FilePath& path) override {
    TRACE_EVENT0("ZipFileCreator", "DirectoryExists");
    const FileInfo& file_info = GetFileInfo(path);
    return file_info.is_directory;
  }

  std::vector<DirectoryContentEntry> ListDirectoryContent(
      const base::FilePath& dir_path) override {
    TRACE_EVENT0("ZipFileCreator", "ListDirectoryContent");
    DCHECK(dir_path.IsAbsolute());

    std::vector<DirectoryContentEntry> results;
    filesystem::mojom::FileError error;
    filesystem::mojom::DirectoryPtr dir_mojo;
    filesystem::mojom::DirectoryPtr* dir_mojo_ptr;
    if (source_dir_ == dir_path) {
      dir_mojo_ptr = &source_dir_mojo_;
    } else {
      base::FilePath relative_path = GetRelativePath(dir_path);
      source_dir_mojo_->OpenDirectory(
          relative_path.value(), mojo::MakeRequest(&dir_mojo),
          filesystem::mojom::kFlagRead | filesystem::mojom::kFlagOpen, &error);
      if (error != filesystem::mojom::FileError::OK) {
        LOG(ERROR) << "Failed to open " << dir_path.value() << " error "
                   << error;
        return results;
      }
      dir_mojo_ptr = &dir_mojo;
    }

    base::Optional<std::vector<filesystem::mojom::DirectoryEntryPtr>>
        directory_contents;
    (*dir_mojo_ptr)->Read(&error, &directory_contents);
    if (error != filesystem::mojom::FileError::OK) {
      LOG(ERROR) << "Failed to list content of " << dir_path.value()
                 << " error " << error;
    }
    if (directory_contents) {
      results.reserve(directory_contents->size());
      for (const filesystem::mojom::DirectoryEntryPtr& entry :
           *directory_contents) {
        base::FilePath path = dir_path.Append(entry->name);
        bool is_directory =
            entry->type == filesystem::mojom::FsFileType::DIRECTORY;
        results.push_back(DirectoryContentEntry(path, is_directory));
      }
    }
    return results;
  }

  base::Time GetLastModifiedTime(const base::FilePath& path) override {
    TRACE_EVENT0("ZipFileCreator", "GetLastModifiedTime");
    const FileInfo& file_info = GetFileInfo(path);
    return base::Time::FromDoubleT(file_info.last_modified_time);
  }

  const FileInfo& GetFileInfo(const base::FilePath& absolute_path) {
    std::map<base::FilePath, FileInfo>::const_iterator iter =
        file_info_.find(absolute_path);
    if (iter != file_info_.end())
      return iter->second;

    base::FilePath relative_path = GetRelativePath(absolute_path);
    filesystem::mojom::FileError error;
    filesystem::mojom::FileInformationPtr file_info_mojo;
    source_dir_mojo_->StatFile(relative_path.value(), &error, &file_info_mojo);
    if (error != filesystem::mojom::FileError::OK) {
      LOG(ERROR) << "Failed to get last modified time of "
                 << absolute_path.value() << " error " << error;
      return null_file_info_;
    }
    FileInfo file_info(
        file_info_mojo->type == filesystem::mojom::FsFileType::DIRECTORY,
        file_info_mojo->mtime);
    file_info_[absolute_path] = file_info;
    return file_info_[absolute_path];
  }

  base::FilePath GetRelativePath(const base::FilePath& path) {
    DCHECK(path.IsAbsolute());
    base::FilePath relative_path;
    bool success = source_dir_.AppendRelativePath(path, &relative_path);
    DCHECK(success);
    return relative_path;
  }

  base::FilePath source_dir_;
  filesystem::mojom::DirectoryPtr source_dir_mojo_;

  std::map<base::FilePath, FileInfo> file_info_;

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

  TRACE_EVENT0("ZipFileCreator", "CreateZipFile");
  zip::ZipParams zip_params(source_dir, zip_file.GetPlatformFile());
  zip_params.set_files_to_zip(source_relative_paths);
  zip_params.set_file_accessor(std::make_unique<MojoFileAccessor>(
      source_dir, std::move(source_dir_mojo)));
  std::move(callback).Run(zip::Zip(zip_params));
}

}  // namespace chrome
