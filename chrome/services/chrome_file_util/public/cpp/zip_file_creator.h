// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_CHROME_FILE_UTIL_PUBLIC_CPP_ZIP_FILE_CREATOR_H_
#define CHROME_SERVICES_CHROME_FILE_UTIL_PUBLIC_CPP_ZIP_FILE_CREATOR_H_

#include <memory>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/services/chrome_file_util/public/interfaces/zip_file_creator.mojom.h"

namespace service_manager {
class Connector;
}

namespace chrome {

// ZipFileCreator creates a ZIP file from a specified list of files and
// directories under a common parent directory. This is done in a sandboxed
// utility process to protect the browser process from handling arbitrary
// input data from untrusted sources.
class ZipFileCreator : public base::RefCountedThreadSafe<ZipFileCreator> {
 public:
  typedef base::Callback<void(bool)> ResultCallback;

  // Creates a zip file from the specified list of files and directories.
  ZipFileCreator(const ResultCallback& callback,
                 const base::FilePath& src_dir,
                 const std::vector<base::FilePath>& src_relative_paths,
                 const base::FilePath& dest_file);

  // Starts creating the zip file. Must be called from the UI thread.
  // The result will be passed to |callback|. After the task is finished
  // and |callback| is run, ZipFileCreator instance is deleted.
  void Start(service_manager::Connector* connector);

 private:
  friend class base::RefCountedThreadSafe<ZipFileCreator>;
  class BindDirectoryParam;

  ~ZipFileCreator();

  // Called after the dest_file |file| is opened on the blocking pool to
  // create the zip file in it using a sandboxed utility process.
  void CreateZipFile(service_manager::Connector* connector, base::File file);

  void BindDirectory(scoped_refptr<BindDirectoryParam> param);
  void OnDirectoryBound(service_manager::Connector* connector,
                        base::File file,
                        scoped_refptr<BindDirectoryParam> param);

  void ReportDone(bool success);

  // The callback.
  ResultCallback callback_;

  // The source directory for input files.
  base::FilePath src_dir_;

  // The list of source files paths to be included in the zip file.
  // Entries are relative paths under directory |src_dir_|.
  std::vector<base::FilePath> src_relative_paths_;

  scoped_refptr<base::SequencedTaskRunner> directory_task_runner_;

  // The output zip file.
  base::FilePath dest_file_;

  chrome::mojom::ZipFileCreatorPtr zip_file_creator_ptr_;

  DISALLOW_COPY_AND_ASSIGN(ZipFileCreator);
};

}  // namespace chrome

#endif  // CHROME_SERVICES_CHROME_FILE_UTIL_PUBLIC_CPP_ZIP_FILE_CREATOR_H_
