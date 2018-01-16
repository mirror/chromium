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

enum { kMaxLogEntries = 100, kMaxMutexWaitMs = 5000 };

// Mutex for log access;
HANDLE g_log_mutex = nullptr;

// This structure will be translated into LogEntry when draining log.
struct LogEntryInternal {
  uint8_t basename_hash[elf_sha1::kSHA1Length];
  uint8_t code_id_hash[elf_sha1::kSHA1Length];
  std::string full_path;
};

// Holds third-party IME information.
class Log {
 public:
  // Move constructor
  Log(Log&&) noexcept = default;
  // Move assignment
  Log& operator=(Log&&) noexcept = default;
  // Constructor
  Log(bool blocked) : blocked_(blocked) {}

  // Returns the size in bytes of the full log, in terms of LogEntry structs.
  // I.e. how many bytes would a provided buffer need to be to DrainLog().
  DWORD GetFullLogSize() {
    DWORD size = 0;
    for (auto entry : log_) {
      size += GetLogEntrySize(static_cast<DWORD>(entry.full_path.size()));
    }
    return size;
  }

  // Add a LogEntryInternal to the log.  Take ownership of the argument.
  // - g_log_mutex should be held before calling this function.
  void AddEntry(LogEntryInternal&& entry) {
    // Sanity checks.  If load blocked, do not add duplicate logs.
    if (log_.size() == kMaxLogEntries ||
        (blocked_ && ContainsEntry(entry.basename_hash, entry.code_id_hash))) {
      return;
    }
    log_.push_back(std::move(entry));
  }

  // Remove |count| entries from start of log vector.
  // - g_log_mutex should be held before calling this function.
  // - More efficient to take a chunk off the vector once, instead of one entry
  //   at a time.
  void DequeueEntries(DWORD count) {
    assert(count <= log_.size());
    log_.erase(log_.begin(), log_.begin() + count);
  }

  // Get a (read-only) pointer to the entry at |index| of the Log.
  // - Returns false if no entry exists at |index|.
  bool GetEntryAt(size_t index, const LogEntryInternal** entry) {
    if (log_.size() <= index)
      return false;
    *entry = &log_[index];
    return true;
  }

 private:
  // Logs are currently unordered, so just loop.
  // - Returns true if the given hashes already exist in the log.
  bool ContainsEntry(const uint8_t* basename_hash,
                     const uint8_t* code_id_hash) {
    for (auto entry : log_) {
      if (!elf_sha1::CompareHashes(basename_hash, entry.basename_hash) &&
          !elf_sha1::CompareHashes(code_id_hash, entry.code_id_hash)) {
        return true;
      }
    }
    return false;
  }

  // Is this a Log of blocked or allowed module load attempts?
  bool blocked_;
  std::vector<LogEntryInternal> log_;

  // DISALLOW_COPY_AND_ASSIGN(Log);
  Log(const Log&) = delete;
  Log& operator=(const Log&) = delete;
};

// NOTE: these "globals" are only initialized once during InitLogs().
// NOTE: they are wrapped in functions to prevent exit-time dtors.
Log& GetBlockedLog() {
  static Log* const blocked_log = new Log(true);
  return *blocked_log;
}

Log& GetAllowedLog() {
  static Log* const allowed_log = new Log(false);
  return *allowed_log;
}

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// Converts a given LogEntryInternal into a LogEntry.
void TranslateEntry(bool blocked, const LogEntryInternal& src, LogEntry* dst) {
  dst->blocked = blocked;
  ::memcpy(dst->basename_hash, src.basename_hash, elf_sha1::kSHA1Length);
  ::memcpy(dst->code_id_hash, src.code_id_hash, elf_sha1::kSHA1Length);

  // Sanity check - there should be no LogEntryInternal with a too long path.
  // LogLoadAttempt() ensures this.
  assert(src.full_path.size() < std::numeric_limits<uint32_t>::max());
  dst->path_len = static_cast<uint32_t>(src.full_path.size());
  ::memcpy(dst->path, src.full_path.c_str(), dst->path_len + 1);
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

// This is called from inside a hook shim, so don't bother with return status.
void LogLoadAttempt(bool blocked,
                    const uint8_t* basename_hash,
                    const uint8_t* code_id_hash,
                    const char* full_image_path) {
  if (::WaitForSingleObject(g_log_mutex, kMaxMutexWaitMs) != WAIT_OBJECT_0)
    return;

  // Build the new log entry.
  LogEntryInternal entry;
  ::memcpy(&entry.basename_hash[0], basename_hash, elf_sha1::kSHA1Length);
  ::memcpy(&entry.code_id_hash[0], code_id_hash, elf_sha1::kSHA1Length);

  // Only store full path if the module was allowed to load.
  if (!blocked) {
    entry.full_path = full_image_path;
    // Edge condition.  Ensure the path length is <= max(DWORD) - 1.
    if (entry.full_path.size() > std::numeric_limits<DWORD>::max() - 1)
      entry.full_path.resize(std::numeric_limits<DWORD>::max() - 1);
  }

  // Add the new entry.
  Log& log = (blocked ? GetBlockedLog() : GetAllowedLog());
  log.AddEntry(std::move(entry));

  ::ReleaseMutex(g_log_mutex);
}

LogStatus InitLogs() {
  // Debug check: InitLogs should not be called more than once.
  assert(!g_log_mutex);

  // Create unnamed mutex for log access.
  g_log_mutex = ::CreateMutex(nullptr, false, nullptr);
  if (!g_log_mutex)
    return LogStatus::kCreateMutexFailure;

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
  if (::WaitForSingleObject(g_log_mutex, kMaxMutexWaitMs) != WAIT_OBJECT_0)
    return 0;
  Log& blocked = GetBlockedLog();
  Log& allowed = GetAllowedLog();

  DWORD remaining_buffer_size = buffer_size;
  DWORD bytes_written = 0;
  DWORD pop_count = 0;
  DWORD entry_size = 0;
  const LogEntryInternal* entry = nullptr;
  // Drain as many 'blocked' entries as possible.
  for (size_t i = 0; blocked.GetEntryAt(i, &entry); ++i) {
    entry_size =
        GetLogEntrySize(static_cast<uint32_t>(entry->full_path.size()));
    if (remaining_buffer_size < entry_size)
      break;
    LogEntry* temp = reinterpret_cast<LogEntry*>(buffer + bytes_written);
    TranslateEntry(true, *entry, temp);
    // Update counters.
    remaining_buffer_size -= entry_size;
    bytes_written += entry_size;
    ++pop_count;
  }
  blocked.DequeueEntries(pop_count);
  pop_count = 0;

  // Drain as many 'allowed' entries as possible.
  for (size_t i = 0; allowed.GetEntryAt(i, &entry); ++i) {
    entry_size =
        GetLogEntrySize(static_cast<uint32_t>(entry->full_path.size()));
    if (remaining_buffer_size < entry_size)
      break;
    LogEntry* temp = reinterpret_cast<LogEntry*>(buffer + bytes_written);
    TranslateEntry(false, *entry, temp);
    // Update counters.
    remaining_buffer_size -= entry_size;
    bytes_written += entry_size;
    ++pop_count;
  }
  allowed.DequeueEntries(pop_count);

  // If requested, return the remaining logs size.
  if (log_remaining) {
    // Edge case: maximum 32-bit value.
    uint64_t full_size = blocked.GetFullLogSize() + allowed.GetFullLogSize();
    if (full_size > std::numeric_limits<DWORD>::max())
      *log_remaining = std::numeric_limits<DWORD>::max();
    else
      *log_remaining = static_cast<uint32_t>(full_size);
  }

  ::ReleaseMutex(g_log_mutex);

  return bytes_written;
}
