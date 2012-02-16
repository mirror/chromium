// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_FILE_SYSTEM_OPERATION_INTERFACE_H_
#define WEBKIT_FILEAPI_FILE_SYSTEM_OPERATION_INTERFACE_H_

#include "base/file_util_proxy.h"
#include "base/platform_file.h"
#include "base/process.h"

namespace base {
class Time;
}  // namespace base

namespace net {
class URLRequest;
class URLRequestContext;
}  // namespace net

class GURL;

namespace fileapi {

class FileSystemOperation;

// The interface class for FileSystemOperation implementations.
//
// This interface defines file system operations required to implement
// "File API: Directories and System"
// http://www.w3.org/TR/file-system-api/
//
// DESIGN NOTES
//
// This class is designed to
//
// 1) Serve one-time file system operation per instance.  Only one
// method(CreateFile, CreateDirectory, Copy, Move, DirectoryExists,
// GetMetadata, ReadDirectory and Remove) may be called during the
// lifetime of this object and it should be called no more than once.
//
// 2) Be self-destructed, or get deleted via base::Owned() after the
// operation finishes and completion callback is called.
//
// 3) Deliver the results of operations to the client via the callback function
// passed as the last parameter of the method.
//
class FileSystemOperationInterface {
 public:
  virtual ~FileSystemOperationInterface() {}

  // Used for CreateFile(), etc. |result| is the return code of the operation.
  typedef base::Callback<void(base::PlatformFileError result)> StatusCallback;

  // Used for GetMetadata(). |result| is the return code of the operation,
  // |file_info| is the obtained file info, and |platform_path| is the path
  // of the file.
  typedef base::Callback<void(
      base::PlatformFileError result,
      const base::PlatformFileInfo& file_info,
      const FilePath& platform_path)> GetMetadataCallback;

  // Used for OpenFile(). |result| is the return code of the operation.
  typedef base::Callback<void(
      base::PlatformFileError result,
      base::PlatformFile file,
      base::ProcessHandle peer_handle)> OpenFileCallback;

  // Used for ReadDirectory(). |result| is the return code of the operation,
  // |file_list| is the list of files read, and |has_more| is true if some files
  // are yet to be read.
  typedef base::Callback<void(
      base::PlatformFileError result,
      const std::vector<base::FileUtilProxy::Entry>& file_list,
      bool has_more)> ReadDirectoryCallback;

  // Used for Write().
  typedef base::Callback<void(base::PlatformFileError result,
                              int64 bytes,
                              bool complete)> WriteCallback;

  // Creates a file at |path|. If |exclusive| is true, an error is raised
  // in case a file is already present at the URL.
  virtual void CreateFile(const GURL& path,
                          bool exclusive,
                          const StatusCallback& callback) = 0;

  // Creates a directory at |path|. If |exclusive| is true, an error is
  // raised in case a directory is already present at the URL. If
  // |recursive| is true, create parent directories as needed just like
  // mkdir -p does.
  virtual void CreateDirectory(const GURL& path,
                               bool exclusive,
                               bool recursive,
                               const StatusCallback& callback) = 0;

  // Copes a file or directory from |src_path| to |dest_path|. If
  // |src_path| is a directory, the contents of |src_path| are copied to
  // |dest_path| recursively. A new file or directory is created at
  // |dest_path| as needed.
  virtual void Copy(const GURL& src_path,
                    const GURL& dest_path,
                    const StatusCallback& callback) = 0;

  // Moves a file or directory from |src_path| to |dest_path|. A new file
  // or directory is created at |dest_path| as needed.
  virtual void Move(const GURL& src_path,
                    const GURL& dest_path,
                    const StatusCallback& callback) = 0;

  // Checks if a directory is present at |path|.
  virtual void DirectoryExists(const GURL& path,
                               const StatusCallback& callback) = 0;

  // Checks if a file is present at |path|.
  virtual void FileExists(const GURL& path,
                          const StatusCallback& callback) = 0;

  // Gets the metadata of a file or directory at |path|.
  virtual void GetMetadata(const GURL& path,
                           const GetMetadataCallback& callback) = 0;

  // Reads contents of a directory at |path|.
  virtual void ReadDirectory(const GURL& path,
                             const ReadDirectoryCallback& callback) = 0;

  // Removes a file or directory at |path|. If |recursive| is true, remove
  // all files and directories under the directory at |path| recursively.
  virtual void Remove(const GURL& path, bool recursive,
                      const StatusCallback& callback) = 0;

  // Writes contents of |blob_url| to |path| at |offset|.
  // |url_request_context| is used to read contents in |blob_url|.
  virtual void Write(const net::URLRequestContext* url_request_context,
                     const GURL& path,
                     const GURL& blob_url,
                     int64 offset,
                     const WriteCallback& callback) = 0;

  // Truncates a file at |path| to |length|. If |length| is larger than
  // the original file size, the file will be extended, and the extended
  // part is filled with null bytes.
  virtual void Truncate(const GURL& path, int64 length,
                        const StatusCallback& callback) = 0;

  // Tries to cancel the current operation [we support cancelling write or
  // truncate only]. Reports failure for the current operation, then reports
  // success for the cancel operation itself via the |cancel_dispatcher|.
  //
  // E.g. a typical cancel implementation would look like:
  //
  //   virtual void SomeOperationImpl::Cancel(
  //       const StatusCallback& cancel_callback) {
  //     // Abort the current inflight operation first.
  //     ...
  //
  //     // Dispatch ABORT error for the current operation by invoking
  //     // the callback function for the ongoing operation,
  //     operation_callback.Run(base::PLATFORM_FILE_ERROR_ABORT, ...);
  //
  //     // Dispatch 'success' for the cancel (or dispatch appropriate
  //     // error code with DidFail() if the cancel has somehow failed).
  //     cancel_callback.Run(base::PLATFORM_FILE_OK);
  //   }
  //
  // Note that, for reporting failure, the callback function passed to a
  // cancellable operations are kept around with the operation instance
  // (as |operation_callback_| in the code example).
  virtual void Cancel(const StatusCallback& cancel_callback) = 0;

  // Modifies timestamps of a file or directory at |path| with
  // |last_access_time| and |last_modified_time|. The function DOES NOT
  // create a file unlike 'touch' command on Linux.
  //
  // This function is used only by Pepper as of writing.
  virtual void TouchFile(const GURL& path,
                         const base::Time& last_access_time,
                         const base::Time& last_modified_time,
                         const StatusCallback& callback) = 0;

  // Opens a file at |path| with |file_flags|, where flags are OR'ed
  // values of base::PlatformFileFlags.
  //
  // |peer_handle| is the process handle of a pepper plugin process, which
  // is necessary for underlying IPC calls with Pepper plugins.
  //
  // This function is used only by Pepper as of writing.
  virtual void OpenFile(const GURL& path,
                        int file_flags,
                        base::ProcessHandle peer_handle,
                        const OpenFileCallback& callback) = 0;

  // For downcasting to FileSystemOperation.
  // TODO(kinuko): this hack should go away once appropriate upload-stream
  // handling based on element types is supported.
  virtual FileSystemOperation* AsFileSystemOperation() = 0;
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_FILE_SYSTEM_OPERATION_INTERFACE_H_
