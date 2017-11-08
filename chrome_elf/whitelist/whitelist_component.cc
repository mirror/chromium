// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_component.h"

#include "windows.h"

#include <assert.h>

#include <algorithm>

#include "chrome/install_static/user_data_dir.h"
#include "chrome_elf/sha1/sha1.h"
#include "chrome_elf/whitelist/whitelist_component_format.h"

namespace whitelist {
namespace {

constexpr wchar_t kComponentSubdir[] = L"\\ThirdPartyModuleList64";
constexpr wchar_t kComponentFilename[] = L"\\dbfile.txt";

// NOTE: these "globals" are only initialized once during InitComponent().
// NOTE: they are wrapped in a function to prevent exit-time dtors.
std::wstring* GetComponentFilePath() {
  static std::wstring* const component_path = new std::wstring();
  return component_path;
}

PackedWhitelistModule** GetComponentArray() {
  static PackedWhitelistModule* component_array = nullptr;
  return &component_array;
}

size_t* GetComponentArraySize() {
  static size_t component_array_size = 0;
  return &component_array_size;
}

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// Returns -1 if |first| < |second|
// Returns 0 if |first| == |second|
// Returns 1 if |first| > |second|
int CompareHashes(const uint8_t* first, const uint8_t* second) {
  // Compare bytes, high-order to low-order.
  for (size_t i = 0; i < elf_sha1::kSHA1Length; ++i) {
    if (first[i] < second[i])
      return -1;
    else if (first[i] > second[i])
      return 1;
    // else they are equal, continue;
  }

  return 0;
}

// Binary predicate compare function for use with
// std::equal_range/std::is_sorted. Must return TRUE if lhs < rhs.
bool HashBinaryPredicate(const PackedWhitelistModule& lhs,
                         const PackedWhitelistModule& rhs) {
  if (CompareHashes(lhs.basename_hash, rhs.basename_hash) < 0)
    return true;

  return false;
}

// Given a component file opened for read, pull in the whitelist database.
bool ReadInArray(HANDLE file) {
  PackedWhitelistMetadata metadata;
  DWORD bytes_read = 0;

  if (!::ReadFile(file, &metadata, sizeof(PackedWhitelistMetadata), &bytes_read,
                  FALSE) ||
      bytes_read != sizeof(PackedWhitelistMetadata)) {
    return false;
  }

  // Careful of versioning.  For now, only support the latest version.
  PackedWhitelistVersion format_version = metadata.version;
  if (format_version != PackedWhitelistVersion::kLatest)
    return false;

  size_t* module_array_size = GetComponentArraySize();
  PackedWhitelistModule** module_array = GetComponentArray();

  *module_array_size = metadata.module_count;
  // Check for size 0.
  if (!*module_array_size)
    return true;

  DWORD buffer_size = *module_array_size * sizeof(PackedWhitelistModule);
  *module_array =
      reinterpret_cast<PackedWhitelistModule*>(new BYTE[buffer_size]);

  if (!::ReadFile(file, *module_array, buffer_size, &bytes_read, FALSE) ||
      bytes_read != buffer_size) {
    return false;
  }

#if !defined(NDEBUG)
  // Debug check the array is sorted.
  assert(std::is_sorted(*module_array, *module_array + *module_array_size,
                        HashBinaryPredicate));
#endif

  return true;
}

// Example file location, relative to user data dir.
// %localappdata% / Google / Chrome SxS / User Data / ThirdPartyModuleList64 /
//
// TODO(pennymac): Missing or malformed component file shouldn't happen.
//                 Possible UMA log in future.
ComponentStatus InitComponentFile() {
  std::wstring* file_path = GetComponentFilePath();
  if (!install_static::GetUserDataDirectory(file_path, nullptr))
    return ComponentStatus::kUserDataDirFail;
  file_path->append(kComponentSubdir);
  file_path->append(kComponentFilename);

  // See if file exists.  INVALID_HANDLE_VALUE alert!
  HANDLE component_file =
      ::CreateFileW(file_path->c_str(), FILE_READ_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (component_file == INVALID_HANDLE_VALUE)
    return ComponentStatus::kNoComponentFile;

  bool success = ReadInArray(component_file);
  ::CloseHandle(component_file);

  return (success) ? ComponentStatus::kSuccess
                   : ComponentStatus::kInitArrayFail;
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

bool WhitelistLookup(const std::string& basename,
                     DWORD image_size,
                     DWORD time_date_stamp) {
  size_t* wl_array_size = GetComponentArraySize();
  PackedWhitelistModule** wl_array = GetComponentArray();

  if (!wl_array_size)
    return false;

  // Max hex 32-bit value is 8 characters long.  2*8+1.
  char buffer[17] = {};
  ::snprintf(buffer, sizeof(buffer), "%08lX%lx", time_date_stamp, image_size);
  std::string code_id(buffer);
  code_id = elf_sha1::SHA1HashString(code_id);
  std::string basename_hash = elf_sha1::SHA1HashString(basename);
  PackedWhitelistModule target = {};
  ::memcpy(target.basename_hash, basename_hash.c_str(), elf_sha1::kSHA1Length);
  ::memcpy(target.code_id_hash, code_id.c_str(), elf_sha1::kSHA1Length);

  // Binary search for primary hash (basename).  There can be more than one
  // match.
  auto pair = std::equal_range(*wl_array, *wl_array + *wl_array_size, target,
                               HashBinaryPredicate);

  // Search for secondary hash.
  for (PackedWhitelistModule* i = pair.first; i != pair.second; ++i) {
    if (!CompareHashes(target.code_id_hash, i->code_id_hash))
      return true;
  }

  // No match.
  return false;
}

std::wstring GetComponentPathUsed() {
  return *GetComponentFilePath();
}

// Grab the latest component whitelist.
// Keep debug asserts for unexpected behaviour contained in here.
ComponentStatus InitComponent() {
  // Debug check: InitIMEs should not be called more than once.

  InitComponentFile();

  return ComponentStatus::kSuccess;
}

}  // namespace whitelist
