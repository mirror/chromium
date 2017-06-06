// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/shell_extension_enumerator_win.h"

#include "base/files/file.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
#include "chrome/browser/conflicts/module_info_util_win.h"

namespace {

void ReadShellExtensions(HKEY parent,
                         std::vector<base::FilePath>* shell_extensions) {
  for (base::win::RegistryValueIterator iter(
           parent, ShellExtensionEnumerator::kShellExtensionRegistryKey);
       iter.Valid(); ++iter) {
    base::string16 key = base::StringPrintf(
        ShellExtensionEnumerator::kClassIdRegistryKeyFormat, iter.Name());

    base::win::RegKey clsid;
    if (clsid.Open(HKEY_CLASSES_ROOT, key.c_str(), KEY_READ) != ERROR_SUCCESS)
      continue;

    base::string16 dll;
    if (clsid.ReadValue(L"", &dll) != ERROR_SUCCESS)
      continue;

    shell_extensions->push_back(base::FilePath(dll));
  }
}

void EnumerateShellExtensionsImpl(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const base::Callback<void(const base::FilePath&, uint32_t, uint32_t)>&
        callback) {
  auto shell_extension_paths =
      ShellExtensionEnumerator::GetShellExtensionPaths();
  for (size_t i = 0; i < shell_extension_paths.size(); i++) {
    uint32_t size_of_image = 0u;
    uint32_t time_date_stamp = 0u;
    ShellExtensionEnumerator::GetModuleImageSizeAndTimeDateStamp(
        shell_extension_paths[i], &size_of_image, &time_date_stamp);

    task_runner->PostTask(
        FROM_HERE, base::Bind(callback, shell_extension_paths[i], size_of_image,
                              time_date_stamp));
  }
}

}  // namespace

// static
const wchar_t ShellExtensionEnumerator::kShellExtensionRegistryKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell "
    L"Extensions\\Approved";

const wchar_t ShellExtensionEnumerator::kClassIdRegistryKeyFormat[] =
    L"CLSID\\%ls\\InProcServer32";

ShellExtensionEnumerator::ShellExtensionEnumerator(
    const OnShellExtensionEnumeratedCallback&
        on_shell_extension_enumerated_callback)
    : on_shell_extension_enumerated_callback_(
          on_shell_extension_enumerated_callback),
      weak_ptr_factory_(this) {
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(
          &EnumerateShellExtensionsImpl, base::SequencedTaskRunnerHandle::Get(),
          base::Bind(&ShellExtensionEnumerator::OnShellExtensionEnumerated,
                     weak_ptr_factory_.GetWeakPtr())));
}

ShellExtensionEnumerator::~ShellExtensionEnumerator() = default;

// static
std::vector<base::FilePath> ShellExtensionEnumerator::GetShellExtensionPaths() {
  base::ThreadRestrictions::AssertIOAllowed();

  std::vector<base::FilePath> shell_extension_paths;
  ReadShellExtensions(HKEY_LOCAL_MACHINE, &shell_extension_paths);
  ReadShellExtensions(HKEY_CURRENT_USER, &shell_extension_paths);

  return shell_extension_paths;
}

// static
void ShellExtensionEnumerator::GetModuleImageSizeAndTimeDateStamp(
    const base::FilePath& path,
    uint32_t* size_of_image,
    uint32_t* time_date_stamp) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  DCHECK(file.IsValid());

  // Get NT header address.
  uint64_t address = offsetof(IMAGE_DOS_HEADER, e_lfanew);
  LONG e_lfanew = 0;
  file.Read(address, reinterpret_cast<char*>(&e_lfanew), sizeof(e_lfanew));

  // Get SizeOfImage.
  uint64_t size_of_image_address = e_lfanew +
                                   offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
                                   offsetof(IMAGE_OPTIONAL_HEADER, SizeOfImage);
  file.Read(size_of_image_address, reinterpret_cast<char*>(size_of_image),
            sizeof(*size_of_image));

  // Get TimeDateStamp.
  uint64_t time_date_stamp_address = e_lfanew +
                                     offsetof(IMAGE_NT_HEADERS, FileHeader) +
                                     offsetof(IMAGE_FILE_HEADER, TimeDateStamp);
  file.Read(time_date_stamp_address, reinterpret_cast<char*>(time_date_stamp),
            sizeof(*time_date_stamp));
}

void ShellExtensionEnumerator::OnShellExtensionEnumerated(
    const base::FilePath& path,
    uint32_t size_of_image,
    uint32_t time_date_stamp) {
  on_shell_extension_enumerated_callback_.Run(path, size_of_image,
                                              time_date_stamp);
}
