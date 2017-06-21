// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/ime_enumerator_win.h"

#include <algorithm>
#include <iterator>

#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/win/registry.h"
#include "chrome/browser/conflicts/module_info_util_win.h"

namespace {

// Returns true if |ime_guid| is the GUID of a built-in Microsoft IME.
bool IsMicrosoftIme(const wchar_t* ime_guid) {
  // This list was provided by Microsoft.
  const wchar_t* kMicrosoftImeGuids[] = {
      L"{0000897b-83df-4b96-be07-0fb58b01c4a4}",
      L"{03b5835f-f03c-411b-9ce2-aa23e1171e36}",
      L"{07eb03d6-b001-41df-9192-bf9b841ee71f}",
      L"{3697c5fa-60dd-4b56-92d4-74a569205c16}",
      L"{531fdebf-9b4c-4a43-a2aa-960e8fcdc732}",
      L"{6a498709-e00b-4c45-a018-8f9e4081ae40}",
      L"{78cb5b0e-26ed-4fcc-854c-77e8f3d1aa80}",
      L"{81d4e9c9-1d3b-41bc-9e6c-4b40bf79e35e}",
      L"{8613e14c-d0c0-4161-ac0f-1dd2563286bc}",
      L"{a028ae76-01b1-46c2-99c4-acd9858ae02f}",
      L"{a1e2b86b-924a-4d43-80f6-8a820df7190f}",
      L"{ae6be008-07fb-400d-8beb-337a64f7051f}",
      L"{b115690a-ea02-48d5-a231-e3578d2fdf80}",
      L"{c1ee01f2-b3b6-4a6a-9ddd-e988c088ec82}",
      L"{dcbd6fa8-032f-11d3-b5b1-00c04fc324a1}",
      L"{e429b25a-e5d3-4d1f-9be3-0c608477e3a1}",
      L"{f25e9f57-2fc8-4eb3-a41a-cce5f08541e6}",
      L"{f89e9e58-bd2f-4008-9ac2-0f816c09f4ee}",
      L"{fa445657-9379-11d6-b41a-00065b83ee53}",
  };

  return std::binary_search(
      std::begin(kMicrosoftImeGuids), std::end(kMicrosoftImeGuids), ime_guid,
      [](const wchar_t* lhs, const wchar_t* rhs) {
        return base::CompareCaseInsensitiveASCII(lhs, rhs) == -1;
      });
}

}  // namespace

// static
const wchar_t ImeEnumerator::kImeRegistryKey[] =
    L"SOFTWARE\\Microsoft\\CTF\\TIP";

// static
const wchar_t ImeEnumerator::kClassIdRegistryKeyFormat[] =
    L"CLSID\\%ls\\InProcServer32";

ImeEnumerator::ImeEnumerator(
    const OnImeEnumeratedCallback& on_ime_enumerated_callback)
    : on_ime_enumerated_callback_(on_ime_enumerated_callback),
      weak_ptr_factory_(this) {
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(&EnumerateImesOnBlockingSequence,
                 base::SequencedTaskRunnerHandle::Get(),
                 base::Bind(&ImeEnumerator::OnImeEnumerated,
                            weak_ptr_factory_.GetWeakPtr())));
}

ImeEnumerator::~ImeEnumerator() = default;

// static
void ImeEnumerator::EnumerateImesOnBlockingSequence(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const OnImeEnumeratedCallback& callback) {
  for (base::win::RegistryKeyIterator iter(HKEY_LOCAL_MACHINE, kImeRegistryKey);
       iter.Valid(); ++iter) {
    const wchar_t* guid = iter.Name();

    // Skip Microsoft IMEs since Chrome won't do anything about those.
    if (IsMicrosoftIme(guid))
      continue;

    base::FilePath dll;
    if (!GetInprocServerDll(guid, &dll))
      continue;

    uint32_t size_of_image = 0;
    uint32_t time_date_stamp = 0;
    if (!GetModuleImageSizeAndTimeDateStamp(dll, &size_of_image,
                                            &time_date_stamp)) {
      return;
    }

    task_runner->PostTask(
        FROM_HERE, base::Bind(callback, dll, size_of_image, time_date_stamp));
  }
}

void ImeEnumerator::OnImeEnumerated(const base::FilePath& path,
                                    uint32_t size_of_image,
                                    uint32_t time_date_stamp) {
  on_ime_enumerated_callback_.Run(path, size_of_image, time_date_stamp);
}

bool ImeEnumerator::GetInprocServerDll(const wchar_t* guid,
                                       base::FilePath* dll) {
  // Look into the view that matches the bitness of the exe first.
  REGSAM initial_view = KEY_WOW64_32KEY;
#if _WIN64
  initial_view = KEY_WOW64_64KEY;
#endif

  base::string16 key_name =
      base::StringPrintf(ImeEnumerator::kClassIdRegistryKeyFormat, guid);

  base::win::RegKey registry_key;
  base::string16 value;
  if (registry_key.Open(HKEY_CLASSES_ROOT, key_name.c_str(),
                        KEY_READ | initial_view) == ERROR_SUCCESS &&
      registry_key.ReadValue(L"", &value) == ERROR_SUCCESS) {
    *dll = base::FilePath(value);
    return true;
  }

  // Close first because another view is being accessed from the same instance.
  registry_key.Close();

  REGSAM backup_view =
      initial_view == KEY_WOW64_32KEY ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
  if (registry_key.Open(HKEY_CLASSES_ROOT, key_name.c_str(),
                        KEY_READ | backup_view) == ERROR_SUCCESS &&
      registry_key.ReadValue(L"", &value) == ERROR_SUCCESS) {
    *dll = base::FilePath(value);
    return true;
  }

  return false;
}
