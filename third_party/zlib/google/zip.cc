// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/zlib/google/zip.h"

#include <list>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "third_party/zlib/google/zip_internal.h"
#include "third_party/zlib/google/zip_reader.h"

#if defined(USE_SYSTEM_MINIZIP)
#include <minizip/unzip.h>
#include <minizip/zip.h>
#else
#include "third_party/zlib/contrib/minizip/unzip.h"
#include "third_party/zlib/contrib/minizip/zip.h"
#endif

namespace zip {

namespace {

bool AddFileToZip(zipFile zip_file,
                  const base::FilePath& src_dir,
                  FileAccessor* file_accessor) {
  base::File file = file_accessor->OpenFileForReading(src_dir);
  if (!file.IsValid()) {
    DLOG(ERROR) << "Could not open file for path " << src_dir.value();
    return false;
  }

  int num_bytes;
  char buf[zip::internal::kZipBufSize];
  do {
    num_bytes = file.ReadAtCurrentPos(buf, zip::internal::kZipBufSize);
    if (num_bytes > 0) {
      if (ZIP_OK != zipWriteInFileInZip(zip_file, buf, num_bytes)) {
        DLOG(ERROR) << "Could not write data to zip for path "
                    << src_dir.value();
        return false;
      }
    }
  } while (num_bytes > 0);

  return true;
}

bool AddEntryToZip(zipFile zip_file,
                   const base::FilePath& path,
                   const base::FilePath& root_path,
                   FileAccessor* file_accessor) {
  base::FilePath relative_path;
  bool result = root_path.AppendRelativePath(path, &relative_path);
  DCHECK(result);
  std::string str_path = relative_path.AsUTF8Unsafe();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&str_path, 0u, "\\", "/");
#endif

  bool is_directory = file_accessor->DirectoryExists(path);
  if (is_directory)
    str_path += "/";

  zip_fileinfo file_info = zip::internal::GetFileInfoForZipping(path);
  if (!zip::internal::ZipOpenNewFileInZip(zip_file, str_path, &file_info))
    return false;

  bool success = true;
  if (!is_directory) {
    success = AddFileToZip(zip_file, path, file_accessor);
  }

  if (ZIP_OK != zipCloseFileInZip(zip_file)) {
    DLOG(ERROR) << "Could not close zip file entry " << str_path;
    return false;
  }

  return success;
}

bool ExcludeNoFilesFilter(const base::FilePath& file_path) {
  return true;
}

bool ExcludeHiddenFilesFilter(const base::FilePath& file_path) {
  return file_path.BaseName().value()[0] != '.';
}

bool IsHiddenFile(const base::FilePath& file_path) {
  return file_path.BaseName().value()[0] == '.';
}

class FileAccessorImpl : public FileAccessor {
 public:
  ~FileAccessorImpl() override = default;

  base::File OpenFileForReading(const base::FilePath& file) override {
    return base::File(file, base::File::FLAG_OPEN | base::File::FLAG_READ);
  }

  bool DirectoryExists(const base::FilePath& file) override {
    return base::DirectoryExists(file);
  }

  std::vector<DirectoryEntry> ListDirectoryContent(const base::FilePath& dir) {
    std::vector<DirectoryEntry> files;
    base::FileEnumerator file_enumerator(
        dir, false /* recursive */,
        base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
    for (base::FilePath path = file_enumerator.Next(); !path.value().empty();
         path = file_enumerator.Next()) {
      LOG(ERROR) << "** JAY ** LIST DIR=" << path.value();
      files.push_back(DirectoryEntry(path, base::DirectoryExists(path)));
    }
    return files;
  }
};

}  // namespace

ZipParams::ZipParams(const base::FilePath& src_dir,
                     const base::FilePath& dest_file)
    : src_dir_(src_dir),
      dest_file_(dest_file),
      file_accessor_(new FileAccessorImpl()) {}

#if defined(OS_POSIX)
// Does not take ownership of |fd|.
ZipParams::ZipParams(const base::FilePath& src_dir, int dest_fd)
    : src_dir_(src_dir),
      dest_fd_(dest_fd),
      file_accessor_(new FileAccessorImpl()) {}
#endif

bool Zip(const ZipParams& params) {
  DCHECK(params.file_accessor()->DirectoryExists(params.src_dir()));

  zipFile zip_file = nullptr;
#if defined(OS_POSIX)
  int dest_fd = params.dest_fd();
  if (dest_fd != base::kInvalidPlatformFile) {
    zip_file = internal::OpenFdForZipping(dest_fd, APPEND_STATUS_CREATE);
    if (!zip_file) {
      DLOG(ERROR) << "Couldn't create ZIP file for FD " << dest_fd;
      return false;
    }
  }
#endif
  if (!zip_file) {
    const base::FilePath& dest_file = params.dest_file();
    DCHECK(!dest_file.empty());
    zip_file = internal::OpenForZipping(dest_file.AsUTF8Unsafe(),
                                        APPEND_STATUS_CREATE);
    if (!zip_file) {
      DLOG(WARNING) << "Couldn't create ZIP file at path " << dest_file;
      return false;
    }
  }

  bool success = true;
  if (params.files_to_zip().empty()) {
    // Using a list so we can call push_back while iterating.
    std::list<FileAccessor::DirectoryEntry> entries;
    entries.push_back(
        FileAccessor::DirectoryEntry(params.src_dir(), true /* is directory*/));
    const FilterCallback& filter_callback = params.filter_callback();
    for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
      const base::FilePath& entry_path = iter->first;
      if ((!params.include_hidden_files() && IsHiddenFile(entry_path)) ||
          (filter_callback && !filter_callback.Run(entry_path))) {
        continue;
      }

      if (iter != entries.begin()  // Exclude the root dir.
          && !AddEntryToZip(zip_file, entry_path, params.src_dir(),
                            params.file_accessor())) {
        success = false;
        break;
      }

      if (iter->second) {
        // It's a directory.
        std::vector<FileAccessor::DirectoryEntry> subentries =
            params.file_accessor()->ListDirectoryContent(entry_path);
        entries.insert(entries.end(), subentries.begin(), subentries.end());
      }
    }
  } else {
    for (std::vector<base::FilePath>::const_iterator iter =
             params.files_to_zip().begin();
         iter != params.files_to_zip().end(); ++iter) {
      const base::FilePath& path = params.src_dir().Append(*iter);
      if (!AddEntryToZip(zip_file, path, params.src_dir(),
                         params.file_accessor())) {
        // TODO(hshi): clean up the partial zip file when error occurs.
        success = false;
        break;
      }
    }
  }

  if (ZIP_OK != zipClose(zip_file, NULL)) {
    DLOG(ERROR) << "Error closing zip file " << params.dest_file().value();
    return false;
  }

  return success;
}

bool Unzip(const base::FilePath& src_file, const base::FilePath& dest_dir) {
  return UnzipWithFilterCallback(src_file, dest_dir,
                                 base::Bind(&ExcludeNoFilesFilter), true);
}

bool UnzipWithFilterCallback(const base::FilePath& src_file,
                             const base::FilePath& dest_dir,
                             const FilterCallback& filter_cb,
                             bool log_skipped_files) {
  ZipReader reader;
  if (!reader.Open(src_file)) {
    DLOG(WARNING) << "Failed to open " << src_file.value();
    return false;
  }
  while (reader.HasMore()) {
    if (!reader.OpenCurrentEntryInZip()) {
      DLOG(WARNING) << "Failed to open the current file in zip";
      return false;
    }
    if (reader.current_entry_info()->is_unsafe()) {
      DLOG(WARNING) << "Found an unsafe file in zip "
                    << reader.current_entry_info()->file_path().value();
      return false;
    }
    if (filter_cb.Run(reader.current_entry_info()->file_path())) {
      if (!reader.ExtractCurrentEntryIntoDirectory(dest_dir)) {
        DLOG(WARNING) << "Failed to extract "
                      << reader.current_entry_info()->file_path().value();
        return false;
      }
    } else if (log_skipped_files) {
      DLOG(WARNING) << "Skipped file "
                    << reader.current_entry_info()->file_path().value();
    }

    if (!reader.AdvanceToNextEntry()) {
      DLOG(WARNING) << "Failed to advance to the next file";
      return false;
    }
  }
  return true;
}

bool ZipWithFilterCallback(const base::FilePath& src_dir,
                           const base::FilePath& dest_file,
                           const FilterCallback& filter_cb) {
  DCHECK(base::DirectoryExists(src_dir));
  ZipParams params(src_dir, dest_file);
  params.set_filter_callback(filter_cb);
  return Zip(params);
}

bool Zip(const base::FilePath& src_dir, const base::FilePath& dest_file,
         bool include_hidden_files) {
  if (include_hidden_files) {
    return ZipWithFilterCallback(
        src_dir, dest_file, base::Bind(&ExcludeNoFilesFilter));
  } else {
    return ZipWithFilterCallback(
        src_dir, dest_file, base::Bind(&ExcludeHiddenFilesFilter));
  }
}

#if defined(OS_POSIX)
bool ZipFiles(const base::FilePath& src_dir,
              const std::vector<base::FilePath>& src_relative_paths,
              int dest_fd) {
  DCHECK(base::DirectoryExists(src_dir));
  ZipParams params(src_dir, dest_fd);
  params.set_files_to_zip(src_relative_paths);
  return Zip(params);
}
#endif  // defined(OS_POSIX)

}  // namespace zip
