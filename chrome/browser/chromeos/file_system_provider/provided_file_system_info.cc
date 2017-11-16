// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"

#include "base/logging.h"

namespace chromeos {
namespace file_system_provider {

ProviderId::ProviderId(const std::string& provider_id,
                       ProviderType provider_type)
    : id_(provider_id),
      type_(provider_type){
}

std::string ProviderId::GetId(){
  return id_;
}

ProviderId::ProviderType ProviderId::GetType(){
  return type_;
}

bool ProviderId::operator==(ProviderId &other){
  return this->GetId() == other.GetId() && this->GetType() == other.GetType();
}

MountOptions::MountOptions()
    : writable(false),
      supports_notify_tag(false),
      opened_files_limit(0) {
}

MountOptions::MountOptions(const std::string& file_system_id,
                           const std::string& display_name)
    : file_system_id(file_system_id),
      display_name(display_name),
      writable(false),
      supports_notify_tag(false),
      opened_files_limit(0) {
}

ProvidedFileSystemInfo::ProvidedFileSystemInfo()
    : writable_(false),
      supports_notify_tag_(false),
      configurable_(false),
      watchable_(false),
      source_(extensions::SOURCE_FILE) {
}

ProvidedFileSystemInfo::ProvidedFileSystemInfo(
    const std::string& provider_id,
    ProviderId::ProviderType provider_type,
    const MountOptions& mount_options,
    const base::FilePath& mount_path,
    bool configurable,
    bool watchable,
    extensions::FileSystemProviderSource source)
    : provider_id_(provider_id, provider_type),
      file_system_id_(mount_options.file_system_id),
      display_name_(mount_options.display_name),
      writable_(mount_options.writable),
      supports_notify_tag_(mount_options.supports_notify_tag),
      opened_files_limit_(mount_options.opened_files_limit),
      mount_path_(mount_path),
      configurable_(configurable),
      watchable_(watchable),
      source_(source) {
  DCHECK_LE(0, mount_options.opened_files_limit);
}

ProvidedFileSystemInfo::ProvidedFileSystemInfo(
    const ProvidedFileSystemInfo& other) = default;

ProvidedFileSystemInfo::~ProvidedFileSystemInfo() {}

}  // namespace file_system_provider
}  // namespace chromeos
