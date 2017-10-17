// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/operations/read_directory.h"

#include <stddef.h>
#include <iostream>
#include <string>
#include <utility>
#include "base/bind.h"
#include "chromeos/dbus/dbus_method_call_status.h"

#include "chrome/browser/chromeos/file_system_provider/operations/get_metadata.h"
#include "chrome/common/extensions/api/file_system_provider.h"
#include "chrome/common/extensions/api/file_system_provider_internal.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/upstart_client.h"

#include <chrono>
using namespace std::chrono;

namespace chromeos {
namespace file_system_provider {
namespace operations {
namespace {

// Convert |input| into |output|. If parsing fails, then returns false.
bool ConvertRequestValueToEntryList(std::unique_ptr<RequestValue> value,
                                    storage::AsyncFileUtil::EntryList* output) {
  using extensions::api::file_system_provider::EntryMetadata;
  using extensions::api::file_system_provider_internal::
      ReadDirectoryRequestedSuccess::Params;

  const Params* params = value->read_directory_success_params();
  if (!params)
    return false;

  for (const EntryMetadata& entry_metadata : params->entries) {
    if (!ValidateIDLEntryMetadata(
            entry_metadata,
            ProvidedFileSystemInterface::METADATA_FIELD_IS_DIRECTORY |
                ProvidedFileSystemInterface::METADATA_FIELD_NAME,
            false /* root_entry */)) {
      return false;
    }

    storage::DirectoryEntry output_entry;
    output_entry.is_directory = *entry_metadata.is_directory;
    output_entry.name = *entry_metadata.name;

    output->push_back(output_entry);
  }

  return true;
}

}  // namespace

ReadDirectory::ReadDirectory(
    extensions::EventRouter* event_router,
    const ProvidedFileSystemInfo& file_system_info,
    const base::FilePath& directory_path,
    const storage::AsyncFileUtil::ReadDirectoryCallback& callback)
    : Operation(event_router, file_system_info),
      directory_path_(directory_path),
      callback_(callback) {
}

ReadDirectory::~ReadDirectory() {
}

void ReadDirectory::HandleEntriesCallback(
    chromeos::DBusMethodCallStatus call_status,
    std::vector<std::string> entries) {
  std::cout << "Handle entries CALLBACK CALLED *** " << std::endl;

  if (call_status == DBUS_METHOD_CALL_FAILURE) {
    std::cout << "DBUS METHOD CALL FAILURE" << std::endl;
  } else {
    std::cout << "DBUS METHOD CALL SUCCESS" << std::endl;
  }

  for (std::vector<std::string>::iterator it = entries.begin();
       it != entries.end(); ++it) {
    std::cout << *it << std::endl;
  }
  {
    int64_t ms = duration_cast<milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
    std::cout << "(T1) read_directory# : " << std::to_string(ms) << std::endl;
    std::to_string(ms);
  }
}

void ReadDirectory::HandleStructCallback(
    chromeos::DBusMethodCallStatus call_status,
    std::vector<chromeos::EntryData> entries) {
  std::cout << "Handle Struct Callback *** " << std::endl;
  if (call_status == DBUS_METHOD_CALL_FAILURE) {
    std::cout << "DBUS METHOD CALL FAILURE" << std::endl;
  } else {
    std::cout << "DBUS METHOD CALL SUCCESS" << std::endl;
  }
  for (std::vector<chromeos::EntryData>::iterator it = entries.begin();
       it != entries.end(); ++it) {
    std::cout << "path: " << it->fullPath << ", size: " << it->size
              << ", name: " << it->name
              << ", mod time: " << it->modification_time
              << ", is directory: " << std::to_string(it->is_directory)
              << std::endl;
  }
  {
    int64_t ms = duration_cast<milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
    std::cout << "(T1.0) read_directory#Handle struct: " << std::to_string(ms)
              << std::endl;
    std::to_string(ms);
  }
}

bool ReadDirectory::Execute(int request_id) {
  using extensions::api::file_system_provider::ReadDirectoryRequestedOptions;

  ReadDirectoryRequestedOptions options;
  options.file_system_id = file_system_info_.file_system_id();
  options.request_id = request_id;
  options.directory_path = directory_path_.AsUTF8Unsafe();

  // Request only is_directory and name metadata fields.
  options.is_directory = true;
  options.name = true;

  std::cout << "Executing read dir *** " << std::endl;
  {
    int64_t ms = duration_cast<milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
    std::cout << "(T0) read_directory#Execute: " << std::to_string(ms)
              << std::endl;
    std::to_string(ms);
  }
  chromeos::DBusThreadManager::Get()
      ->GetSmbClientClient()
      ->CommunicateToService("smb://192.168.0.102/testshare/temp",
                             base::Bind(&ReadDirectory::HandleStructCallback));

  return SendEvent(
      request_id,
      extensions::events::FILE_SYSTEM_PROVIDER_ON_READ_DIRECTORY_REQUESTED,
      extensions::api::file_system_provider::OnReadDirectoryRequested::
          kEventName,
      extensions::api::file_system_provider::OnReadDirectoryRequested::Create(
          options));
}

void ReadDirectory::OnSuccess(int /* request_id */,
                              std::unique_ptr<RequestValue> result,
                              bool has_more) {
  storage::AsyncFileUtil::EntryList entry_list;
  const bool convert_result =
      ConvertRequestValueToEntryList(std::move(result), &entry_list);

  if (!convert_result) {
    LOG(ERROR)
        << "Failed to parse a response for the read directory operation.";
    callback_.Run(base::File::FILE_ERROR_IO,
                  storage::AsyncFileUtil::EntryList(),
                  false /* has_more */);
    return;
  }

  callback_.Run(base::File::FILE_OK, entry_list, has_more);
}

void ReadDirectory::OnError(int /* request_id */,
                            std::unique_ptr<RequestValue> /* result */,
                            base::File::Error error) {
  callback_.Run(
      error, storage::AsyncFileUtil::EntryList(), false /* has_more */);
}

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos
