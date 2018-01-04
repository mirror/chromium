// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_log.h"

#include "windows.h"

#include <vector>

#include <assert.h>

#include "chrome_elf/sha1/sha1.h"
#include "chrome_elf/whitelist/whitelist_packed_format.h"

namespace whitelist {
namespace {

// No concern about concurrency control in chrome_elf.
bool g_initialized = false;

enum { kMaxLogEntries = 100, kMaxMutexWait = 5000 };

// Mutex for log access;
HANDLE g_log_mutex = nullptr;

// This structure will be translated into LogEntry
struct LogEntryInternal {
  uint8_t basename_hash[20];
  uint8_t code_id_hash[20];
  std::string full_path;
};

// These sizes should be tracked in terms of LogEntry structs.
// I.e. how many bytes would a provided buffer need to be to DrainLog().
DWORD g_blocked_log_size = 0;
DWORD g_allowed_log_size = 0;

// NOTE: these "globals" are only initialized once during InitLogs().
// NOTE: they are wrapped in functions to prevent exit-time dtors.
std::vector<LogEntryInternal>& GetBlockedLog() {
  static std::vector<LogEntryInternal>* const blocked_log =
      new std::vector<LogEntryInternal>();
  return *blocked_log;
}

std::vector<LogEntryInternal>& GetAllowedLog() {
  static std::vector<LogEntryInternal>* const allowed_log =
      new std::vector<LogEntryInternal>();
  return *allowed_log;
}

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// Logs are currently unordered, so just loop.
// - Returns true if the given hashes already exist in the |log|.
bool CheckForExistingEntry(const std::vector<LogEntryInternal>& log,
                           const uint8_t* basename_hash,
                           const uint8_t* code_id_hash) {
  for (auto entry : log) {
    if (!elf_sha1::CompareHashes(basename_hash, entry.basename_hash) &&
        !elf_sha1::CompareHashes(code_id_hash, entry.code_id_hash)) {
      return true;
    }
  }

  return false;
}

// Tiny function to keep this logic in one place.
// - |string_length| should not include EoS character.
DWORD CalculateExtraSpaceNeeded(DWORD string_length) {
  // LogEvent.path already has 4 bytes of space due to padding.
  DWORD extra = (string_length > 3) ? (string_length - 3) : 0;

  // The full entry needs to be 32-bit aligned.  Get the modulo, and leave 0
  // alone.
  DWORD align = (sizeof(LogEntry) + extra) % 4;
  if (align) {
    // Flip the remainder, for how many bytes need to be added.
    align = 4 - align;
  }

  return extra + align;
}

// Converts a given LogEntryInternal into a LogEntry.
void TranslateEntry(bool blocked, const LogEntryInternal& src, LogEntry* dst) {
  dst->blocked = blocked ? true : false;
  ::memcpy(dst->basename_hash, src.basename_hash, elf_sha1::kSHA1Length);
  ::memcpy(dst->code_id_hash, src.code_id_hash, elf_sha1::kSHA1Length);
  dst->path_len = src.full_path.size();
  ::strcpy(dst->path, src.full_path.c_str());

  dst->path_extra_size = CalculateExtraSpaceNeeded(dst->path_len);

  // At this point, |dst| entry is size = sizeof(LogEntry) + path_extra_space.
  // And that size is divisible by 4 bytes (32-bit alignment).
  return;
}

// Remove |count| entries from start of log vector.
// - g_log_mutex should be held before calling this function.
void DequeueEntries(bool blocked_log, DWORD count) {
  std::vector<LogEntryInternal>& log =
      blocked_log ? GetBlockedLog() : GetAllowedLog();
  DWORD bytes = 0;
  for (size_t i = 0; i < count; ++i) {
    bytes +=
        sizeof(LogEntry) + CalculateExtraSpaceNeeded(log[i].full_path.size());
  }
  log.erase(log.begin(), log.begin() + count);
  (blocked_log ? g_blocked_log_size : g_allowed_log_size) -= bytes;

  return;
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

void AddLoadAttemptLog(bool blocked,
                       const uint8_t* basename_hash,
                       const uint8_t* code_id_hash,
                       const char* full_image_path) {
  if (::WaitForSingleObject(g_log_mutex, kMaxMutexWait) != WAIT_OBJECT_0)
    return;

  std::vector<LogEntryInternal>& log =
      (blocked ? GetBlockedLog() : GetAllowedLog());

  // If load blocked, do not add duplicate logs.
  if (log.size() > kMaxLogEntries ||
      (blocked && CheckForExistingEntry(log, basename_hash, code_id_hash))) {
    ::ReleaseMutex(g_log_mutex);
    return;
  }

  LogEntryInternal entry = {};
  ::memcpy(&entry.basename_hash, basename_hash, elf_sha1::kSHA1Length);
  ::memcpy(&entry.code_id_hash, code_id_hash, elf_sha1::kSHA1Length);
  // Only store full path if the module was allowed to load.
  if (!blocked)
    entry.full_path = full_image_path;

  // Add the new entry and update the associated log size.
  log.push_back(entry);
  (blocked ? g_blocked_log_size : g_allowed_log_size) +=
      (sizeof(LogEntry) + CalculateExtraSpaceNeeded(entry.full_path.size()));

  ::ReleaseMutex(g_log_mutex);

  return;
}

LogStatus InitLogs() {
  // Debug check: InitLogs should not be called more than once.
  assert(!g_initialized);

  // Create unnamed mutex for log access.
  g_log_mutex = ::CreateMutex(nullptr, false, nullptr);
  if (!g_log_mutex)
    return LogStatus::kCreateMutexFailure;

  g_initialized = true;

  return LogStatus::kSuccess;
}

}  // namespace whitelist

//------------------------------------------------------------------------------
// Exports
// - Function definition in whitelist_packed_format.h
// - Export declared in chrome_elf_[x64|x86].def
//------------------------------------------------------------------------------
using namespace whitelist;

uint32_t DrainLog(uint8_t* buffer,
                  uint32_t buffer_size,
                  uint32_t* log_remaining) {
  if (::WaitForSingleObject(g_log_mutex, kMaxMutexWait) != WAIT_OBJECT_0)
    return 0;
  std::vector<LogEntryInternal>& blocked = GetBlockedLog();
  std::vector<LogEntryInternal>& allowed = GetAllowedLog();

  // Drain as many entries as possible - blocked first.
  // NOTE: struct LogEntry should be used in size calculations, not
  // LogEntryInternal.
  DWORD remaining_buffer_size = buffer_size;
  DWORD bytes_written = 0;
  DWORD pop_count = 0;
  DWORD entry_size = 0;
  for (auto entry : blocked) {
    entry_size =
        sizeof(LogEntry) + CalculateExtraSpaceNeeded(entry.full_path.size());
    if (remaining_buffer_size < entry_size)
      break;
    LogEntry* temp = reinterpret_cast<LogEntry*>(buffer + bytes_written);
    TranslateEntry(true, entry, temp);
    // Update counters.
    remaining_buffer_size -= entry_size;
    bytes_written += entry_size;
    ++pop_count;
  }
  DequeueEntries(true, pop_count);
  pop_count = 0;

  for (auto entry : allowed) {
    entry_size =
        sizeof(LogEntry) + CalculateExtraSpaceNeeded(entry.full_path.size());
    if (remaining_buffer_size < entry_size)
      break;
    LogEntry* temp = reinterpret_cast<LogEntry*>(buffer + bytes_written);
    TranslateEntry(false, entry, temp);
    // Update counters.
    remaining_buffer_size -= entry_size;
    bytes_written += entry_size;
    ++pop_count;
  }
  DequeueEntries(false, pop_count);

  *log_remaining = g_blocked_log_size + g_allowed_log_size;
  ::ReleaseMutex(g_log_mutex);

  return bytes_written;
}
