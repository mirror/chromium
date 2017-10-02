// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_ime.h"

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "chrome_elf/nt_registry/nt_registry.h"

namespace {

//------------------------------------------------------------------------------
// Private defines
//------------------------------------------------------------------------------

// This list was provided by Microsoft.
constexpr const wchar_t* kMicrosoftImeGuids[] = {
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

// Holds third-party IME information.
class IME {
 public:
  IME() : image_size_(0), date_time_stamp_(0) {}
  IME(std::unique_ptr<std::wstring> guid,
      DWORD image_size,
      DWORD date_time_stamp,
      std::unique_ptr<std::wstring> path)
      : image_size_(image_size), date_time_stamp_(date_time_stamp) {
    // Take ownership of the std::wstrings passed in.
    guid_ = std::move(guid);
    path_ = std::move(path);
  }
  ~IME() {}
  DWORD image_size() { return image_size_; }
  DWORD date_time_stamp() { return date_time_stamp_; }
  const wchar_t* guid() { return guid_->c_str(); }
  const wchar_t* path() { return path_->c_str(); }

 private:
  DWORD image_size_;
  DWORD date_time_stamp_;
  std::unique_ptr<std::wstring> guid_;
  std::unique_ptr<std::wstring> path_;

  // DISALLOW_COPY_AND_ASSIGN(IME);
  IME(const IME&) = delete;
  IME& operator=(const IME&) = delete;
};

// List of third-party IMEs registered on system.
// NOTE: this list is only initialized once during InitIMEs().
std::vector<std::unique_ptr<IME>> g_ime_list;

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// Returns true if |ime_guid| is the GUID of a built-in Microsoft IME.
bool IsMicrosoftIme(const wchar_t* ime_guid) {
  assert(ime_guid);

  // Debug check the array is sorted.
  assert(std::is_sorted(std::begin(kMicrosoftImeGuids),
                        std::end(kMicrosoftImeGuids)));

  // The binary predicate (compare function) must return true if the first
  // argument is ordered before the second.
  return std::binary_search(std::begin(kMicrosoftImeGuids),
                            std::end(kMicrosoftImeGuids), ime_guid,
                            [](const wchar_t* lhs, const wchar_t* rhs) {
                              return (::wcsicmp(lhs, rhs) < 0);
                            });
}

// Returns the path to the in-proc server DLL for |guid|.
bool GetInprocServerDllPath(const wchar_t* guid, std::wstring* value) {
  assert(guid && value);

  std::wstring subpath(whitelist::kClassesKey);
  subpath.append(guid);
  subpath.append(L"\\");
  subpath.append(whitelist::kClassesSubkey);
  HANDLE key_handle = nullptr;

  // HKCR is actually a combination view of HKLM and HKCU.  HKLM are defaults
  // for all users and HKCU are user specific and OVERRIDE HKLM classes. So
  // check HKCU first, and only if it doesn't exist there, try HKLM.
  if (!nt::OpenRegKey(nt::HKCU, subpath.c_str(), KEY_QUERY_VALUE, &key_handle,
                      nullptr) &&
      !nt::OpenRegKey(nt::HKLM, subpath.c_str(), KEY_QUERY_VALUE, &key_handle,
                      nullptr)) {
    return false;
  }

  // "(Default)" value string is full path to DLL.
  if (!nt::QueryRegValueSZ(key_handle, L"", value)) {
    nt::CloseRegKey(key_handle);
    return false;
  }

  nt::CloseRegKey(key_handle);
  return true;
}

// Mines DLL information from |path|.
bool GetModuleImageSizeAndTimeDateStamp(const wchar_t* path,
                                        DWORD* size_of_image,
                                        DWORD* time_date_stamp) {
  assert(path && size_of_image && time_date_stamp);

  // Only need the headers.
  constexpr DWORD kPageSize = 4096;
  // Don't zero the buffer - waste of time.
  BYTE buffer[kPageSize];

  // INVALID_HANDLE_VALUE alert.
  HANDLE file =
      ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return false;

  DWORD read = 0;
  if (!::ReadFile(file, &buffer, kPageSize, &read, NULL)) {
    ::CloseHandle(file);
    return false;
  }

  // Done with file.
  ::CloseHandle(file);
  if (read != kPageSize)
    return false;

  // Note: Can't use base_static->pe_image module here, as it doesn't
  // currently handle 32 or 64-bit targets.  It assumes the target PE
  // matches the bitness of itself.
  PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(&buffer);
  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    return false;
  // Right up until the IMAGE_OPTIONAL_HEADER Magic value, 32 and 64 are the
  // same.
  PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
      reinterpret_cast<char*>(dos_header) + dos_header->e_lfanew);
  if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
    return false;

  if (nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    PIMAGE_OPTIONAL_HEADER64 opt_hdr =
        reinterpret_cast<PIMAGE_OPTIONAL_HEADER64>(&nt_headers->OptionalHeader);
    *size_of_image = opt_hdr->SizeOfImage;
  } else if (nt_headers->OptionalHeader.Magic ==
             IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    PIMAGE_OPTIONAL_HEADER32 opt_hdr =
        reinterpret_cast<PIMAGE_OPTIONAL_HEADER32>(&nt_headers->OptionalHeader);
    *size_of_image = opt_hdr->SizeOfImage;
  } else {
    return false;
  }

  // Left until end so as to not touch |time_date_stamp| if any failure.
  *time_date_stamp = nt_headers->FileHeader.TimeDateStamp;

  return true;
}

}  // namespace

namespace whitelist {

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------

// Harvest the registered IMEs in the system.
// Keep debug asserts for unexpected behaviour contained in here.
IMEStatus InitIMEs() {
  // Debug check: InitIMEs should not be called more than once.
  assert(g_ime_list.size() == 0);
  g_ime_list.clear();

  HANDLE key_handle = nullptr;
  NTSTATUS ntstatus = STATUS_UNSUCCESSFUL;

  // Open HKEY_LOCAL_MACHINE, kImeRegistryKey.
  if (!nt::OpenRegKey(nt::HKLM, whitelist::kImeRegistryKey, KEY_READ,
                      &key_handle, &ntstatus)) {
    if (ntstatus == STATUS_OBJECT_NAME_NOT_FOUND)
      return whitelist::WL_MISSING_REG_KEY;
    if (ntstatus == STATUS_ACCESS_DENIED)
      return whitelist::WL_REG_ACCESS_DENIED;
    return whitelist::WL_ERROR_GENERIC;
  }

  ULONG count = 0;
  if (!nt::QueryRegEnumerationInfo(key_handle, &count)) {
    nt::CloseRegKey(key_handle);
    return whitelist::WL_GENERIC_REG_FAILURE;
  }

  for (ULONG i = 0; i < count; i++) {
    // If index is now bad, continue on.
    std::unique_ptr<std::wstring> subkey_name(new std::wstring);
    if (!nt::QueryRegSubkey(key_handle, i, subkey_name.get()))
      continue;

    // Basic verification of expected GUID.
    if (subkey_name->length() != ::wcslen(kMicrosoftImeGuids[0])) {
      assert(false);
      continue;
    }

    // Skip Microsoft IMEs since Chrome won't do anything about those.
    if (IsMicrosoftIme(subkey_name->c_str()))
      continue;

    // Collect DLL info.
    std::unique_ptr<std::wstring> dll_path(new std::wstring);
    if (!GetInprocServerDllPath(subkey_name->c_str(), dll_path.get())) {
      assert(false);
      continue;
    }

    // Mine info out of the DLL.
    DWORD size = 0;
    DWORD date_time = 0;
    if (!GetModuleImageSizeAndTimeDateStamp(dll_path->c_str(), &size,
                                            &date_time)) {
      assert(false);
      continue;
    }

    // For each guid: store guid, size, timestamp, path.
    std::unique_ptr<IME> ime(
        new IME(std::move(subkey_name), size, date_time, std::move(dll_path)));
    g_ime_list.push_back(std::move(ime));
  }

  nt::CloseRegKey(key_handle);
  return whitelist::WL_SUCCESS;
}

}  // namespace whitelist
