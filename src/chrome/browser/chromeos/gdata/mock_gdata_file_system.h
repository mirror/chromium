// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_GDATA_MOCK_GDATA_FILE_SYSTEM_H_
#define CHROME_BROWSER_CHROMEOS_GDATA_MOCK_GDATA_FILE_SYSTEM_H_
#pragma once

#include <string>

#include "chrome/browser/chromeos/gdata/gdata_file_system.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace gdata {

// Mock for GDataFileSystemInterface.
class MockGDataFileSystem : public GDataFileSystemInterface {
 public:
  MockGDataFileSystem();
  virtual ~MockGDataFileSystem();

  // GDataFileSystemInterface overrides.
  MOCK_METHOD0(Initialize, void());
  MOCK_METHOD1(AddObserver, void(Observer* observer));
  MOCK_METHOD1(RemoveObserver, void(Observer* observer));
  MOCK_METHOD0(StartUpdates, void());
  MOCK_METHOD0(StopUpdates, void());
  MOCK_METHOD0(CheckForUpdates, void());
  MOCK_METHOD2(GetFileInfoByResourceId,
               void(const std::string& resource_id,
                    const GetFileInfoWithFilePathCallback& callback));
  MOCK_METHOD2(Search, void(const std::string& search_query,
                            const SearchCallback& callback));
  MOCK_METHOD3(TransferFileFromRemoteToLocal,
               void(const FilePath& local_src_file_path,
                    const FilePath& remote_dest_file_path,
                    const FileOperationCallback& callback));
  MOCK_METHOD3(TransferFileFromLocalToRemote,
               void(const FilePath& local_src_file_path,
                    const FilePath& remote_dest_file_path,
                    const FileOperationCallback& callback));
  MOCK_METHOD2(OpenFile, void(const FilePath& file_path,
                              const OpenFileCallback& callback));
  MOCK_METHOD2(CloseFile, void(const FilePath& file_path,
                              const FileOperationCallback& callback));
  MOCK_METHOD3(Copy, void(const FilePath& src_file_path,
                          const FilePath& dest_file_path,
                          const FileOperationCallback& callback));
  MOCK_METHOD3(Move, void(const FilePath& src_file_path,
                          const FilePath& dest_file_path,
                          const FileOperationCallback& callback));
  MOCK_METHOD3(Remove, void(const FilePath& file_path,
                            bool is_recursive,
                            const FileOperationCallback& callback));
  MOCK_METHOD4(CreateDirectory,
               void(const FilePath& directory_path,
                    bool is_exclusive,
                    bool is_recursive,
                    const FileOperationCallback& callback));
  MOCK_METHOD3(CreateFile,
               void(const FilePath& file_path,
                    bool is_exclusive,
                    const FileOperationCallback& callback));
  MOCK_METHOD3(GetFileByPath,
               void(const FilePath& file_path,
                    const GetFileCallback& get_file_callback,
                    const GetDownloadDataCallback& get_download_data_callback));
  MOCK_METHOD3(GetFileByResourceId,
               void(const std::string& resource_id,
                    const GetFileCallback& get_file_callback,
                    const GetDownloadDataCallback& get_download_data_callback));
  MOCK_METHOD2(UpdateFileByResourceId,
               void(const std::string& resource_id,
                    const FileOperationCallback& callback));
  MOCK_METHOD2(GetFileInfoByPath, void(const FilePath& file_path,
                                       const GetFileInfoCallback& callback));
  MOCK_METHOD2(GetEntryInfoByPath, void(const FilePath& file_path,
                                        const GetEntryInfoCallback& callback));
  MOCK_METHOD2(ReadDirectoryByPath,
               void(const FilePath& file_path,
                    const ReadDirectoryCallback& callback));
  MOCK_METHOD1(RequestDirectoryRefresh,
               void(const FilePath& file_path));
  MOCK_METHOD1(GetAvailableSpace,
               void(const GetAvailableSpaceCallback& callback));
  MOCK_METHOD6(AddUploadedFile,
               void(UploadMode upload_mode,
                    const FilePath& file,
                    DocumentEntry* entry,
                    const FilePath& file_content_path,
                    GDataCache::FileOperationType cache_operation,
                    const base::Closure& callback));
};

}  // namespace gdata

#endif  // CHROME_BROWSER_CHROMEOS_GDATA_MOCK_GDATA_FILE_SYSTEM_H_
