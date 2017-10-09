// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_ZLIB_GOOGLE_ZIP_H_
#define THIRD_PARTY_ZLIB_GOOGLE_ZIP_H_

#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/platform_file.h"
#include "build/build_config.h"

namespace base {
class File;
}

namespace zip {

class FileAccessor {
 public:
  virtual ~FileAccessor() = default;

  using DirectoryEntry = std::pair<base::FilePath, bool /* is directory */>;
  virtual base::File OpenFileForReading(const base::FilePath& file) = 0;
  virtual bool DirectoryExists(const base::FilePath& file) = 0;
  virtual std::vector<DirectoryEntry> ListDirectoryContent(
      const base::FilePath& dir) = 0;
};

class ZipParams {
 public:
  ZipParams(const base::FilePath& src_dir, const base::FilePath& dest_file);
#if defined(OS_POSIX)
  // Does not take ownership of |fd|.
  ZipParams(const base::FilePath& src_dir, int dest_fd);

  int dest_fd() const { return dest_fd_; }
#endif

  const base::FilePath& src_dir() const { return src_dir_; }

  const base::FilePath& dest_file() const { return dest_file_; }

  // Restricts the files actually zipped to the paths listed  in
  // |src_relative_paths|. They must be relative to the |src_dir| passed in the
  // constructor and will be used as the file names in the created zip file. All
  // source paths must be under |src_dir| in the file system hierarchy.
  void set_files_to_zip(const std::vector<base::FilePath>& src_relative_paths) {
    src_files_ = src_relative_paths;
  }
  const std::vector<base::FilePath>& files_to_zip() const { return src_files_; }

  typedef base::Callback<bool(const base::FilePath&)> FilterCallback;
  void set_filter_callback(FilterCallback filter_callback) {
    filter_callback_ = filter_callback;
  }
  const FilterCallback& filter_callback() const { return filter_callback_; }

  void set_include_hidden_files(bool include_hidden_files) {
    include_hidden_files_ = include_hidden_files;
  }
  bool include_hidden_files() const { return include_hidden_files_; }

  // Sets a custom file accessor for file operations. Default is to directly
  // access the files (with fopen and the rest).
  // Useful in cases where runnin in a sandbox process and file access has to go
  // through IPC for example.
  void set_file_accessor(std::unique_ptr<FileAccessor> file_accessor) {
    file_accessor_ = std::move(file_accessor);
  }
  FileAccessor* file_accessor() const { return file_accessor_.get(); }

 private:
  base::FilePath src_dir_;

  base::FilePath dest_file_;
  int dest_fd_ = base::kInvalidPlatformFile;

  // Only the files whose relative path to |src_dir_| are included in the zip
  // file. If this is empty, all files are included.
  std::vector<base::FilePath> src_files_;

  FilterCallback filter_callback_;

  bool include_hidden_files_ = true;

  std::unique_ptr<FileAccessor> file_accessor_;
};

bool Zip(const ZipParams& params);

// Zip the contents of src_dir into dest_file. src_path must be a directory.
// An entry will *not* be created in the zip for the root folder -- children
// of src_dir will be at the root level of the created zip. For each file in
// src_dir, include it only if the callback |filter_cb| returns true. Otherwise
// omit it.
typedef base::Callback<bool(const base::FilePath&)> FilterCallback;
bool ZipWithFilterCallback(const base::FilePath& src_dir,
                           const base::FilePath& dest_file,
                           const FilterCallback& filter_cb);

// Convenience method for callers who don't need to set up the filter callback.
// If |include_hidden_files| is true, files starting with "." are included.
// Otherwise they are omitted.
bool Zip(const base::FilePath& src_dir, const base::FilePath& dest_file,
         bool include_hidden_files);

#if defined(OS_POSIX)
// Zips files listed in |src_relative_paths| to destination specified by file
// descriptor |dest_fd|, without taking ownership of |dest_fd|. The paths listed
// in |src_relative_paths| are relative to the |src_dir| and will be used as the
// file names in the created zip file. All source paths must be under |src_dir|
// in the file system hierarchy.
bool ZipFiles(const base::FilePath& src_dir,
              const std::vector<base::FilePath>& src_relative_paths,
              int dest_fd);
#endif  // defined(OS_POSIX)

// Unzip the contents of zip_file into dest_dir.
// For each file in zip_file, include it only if the callback |filter_cb|
// returns true. Otherwise omit it.
// If |log_skipped_files| is true, files skipped during extraction are printed
// to debug log.
typedef base::Callback<bool(const base::FilePath&)> FilterCallback;
bool UnzipWithFilterCallback(const base::FilePath& zip_file,
                             const base::FilePath& dest_dir,
                             const FilterCallback& filter_cb,
                             bool log_skipped_files);

// Unzip the contents of zip_file into dest_dir.
bool Unzip(const base::FilePath& zip_file, const base::FilePath& dest_dir);

}  // namespace zip

#endif  // THIRD_PARTY_ZLIB_GOOGLE_ZIP_H_
