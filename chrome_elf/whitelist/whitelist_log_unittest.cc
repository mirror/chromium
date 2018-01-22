// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_log.h"

#include <windows.h>

#include <string>

#include "base/macros.h"
#include "chrome_elf/sha1/sha1.h"
#include "chrome_elf/whitelist/whitelist_packed_format.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace whitelist {
namespace {

enum { kWaitTimeoutMs = 5000 };

// Event handle used in notification testing.
HANDLE g_test_event = nullptr;

struct TestEntry {
  uint8_t basename_hash[elf_sha1::kSHA1Length];
  uint8_t code_id_hash[elf_sha1::kSHA1Length];
};

// Sample TestEntries - hashes are the same, except for first bytes.
TestEntry kTestLogs[] = {
    {{0x11, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71},
     {0x22, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71}},
    {{0x33, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71},
     {0x44, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71}},
    {{0x55, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71},
     {0x66, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71}},
    {{0x77, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71},
     {0x88, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71}},
    {{0x99, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71},
     {0xaa, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71}},
    {{0xbb, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71},
     {0xcc, 0xab, 0x9e, 0xa4, 0xbe, 0xf5, 0xf3, 0x6e, 0x7f, 0x20,
      0xc3, 0xaf, 0x63, 0x9c, 0x6f, 0x0e, 0xfe, 0x5f, 0x27, 0x71}},
};

// Be sure to test the padding/alignment issues well here.
const std::string kTestPaths[] = {
    "1", "123", "1234", "12345", "123456", "1234567",
};

static_assert(
    arraysize(kTestLogs) == arraysize(kTestPaths),
    "Some tests currently expect these two arrays to be the same size.");

// Ensure |buffer_size| passed in is the actual bytes written by DrainLog().
void VerifyBuffer(uint8_t* buffer, uint32_t buffer_size) {
  uint32_t total_logs = 0;
  size_t index = 0;
  size_t array_size = arraysize(kTestLogs);

  // Verify against kTestLogs/kTestPaths.  Expect 2 * arraysize(kTestLogs)
  // entries: first half are "blocked", second half are "allowed".
  LogEntry* entry = nullptr;
  uint8_t* tracker = buffer;
  while (tracker < buffer + buffer_size) {
    entry = reinterpret_cast<LogEntry*>(tracker);

    EXPECT_EQ(elf_sha1::CompareHashes(entry->basename_hash,
                                      kTestLogs[index].basename_hash),
              0);
    EXPECT_EQ(elf_sha1::CompareHashes(entry->code_id_hash,
                                      kTestLogs[index].code_id_hash),
              0);
    if (entry->path_len)
      EXPECT_STREQ(entry->path, kTestPaths[index].c_str());

    ++total_logs;
    tracker += GetLogEntrySize(entry->path_len);

    // Roll index back to 0 for second run through kTestLogs, else increment.
    index = (index == array_size - 1) ? 0 : index + 1;
  }
  EXPECT_EQ(total_logs, array_size * 2);

  return;
}

// Helper function to count the number of LogEntries in a buffer returned from
// DrainLog().
uint32_t GetLogCount(uint8_t* buffer, uint32_t bytes_written) {
  LogEntry* entry = nullptr;
  uint8_t* tracker = buffer;
  uint32_t counter = 0;

  while (tracker < buffer + bytes_written) {
    entry = reinterpret_cast<LogEntry*>(tracker);
    ++counter;
    tracker += GetLogEntrySize(entry->path_len);
  }

  return counter;
}

// A thread function used to test log notifications.
// - |lpParameter| is the expected number of LogEntries that should be drained
//   in a successful test run (using the notification API).
// - Returns 0 for successful retrieval of expected number of LogEntries.
DWORD WINAPI NotificationHandler(LPVOID lpParameter) {
  uint32_t logs_expected = *(reinterpret_cast<uint32_t*>(lpParameter));
  uint32_t log_counter = 0;
  DWORD wait_status = WAIT_FAILED;

  // Make a buffer big enough for any possible DrainLog call.
  uint32_t buffer_size = logs_expected * GetLogEntrySize(0);
  uint8_t* buffer = reinterpret_cast<uint8_t*>(new uint8_t[buffer_size]);
  uint32_t bytes_written = 0;

  do {
    wait_status = ::WaitForSingleObject(g_test_event, kWaitTimeoutMs);
    if (wait_status != WAIT_OBJECT_0)
      break;

    bytes_written = DrainLog(buffer, buffer_size, nullptr);
    log_counter += GetLogCount(buffer, bytes_written);
  } while (log_counter < logs_expected);

  delete[] buffer;

  return (log_counter == logs_expected) ? 0 : 1;
}

//------------------------------------------------------------------------------
// Whitelist log tests
//------------------------------------------------------------------------------

// Test successful initialization and module lookup.
TEST(Whitelist, Logs) {
  // Init.
  ASSERT_EQ(InitLogs(), LogStatus::kSuccess);

  for (size_t i = 0; i < arraysize(kTestLogs); ++i) {
    // Add some blocked entries.
    LogLoadAttempt(LogType::kBlocked, kTestLogs[i].basename_hash,
                   kTestLogs[i].code_id_hash, nullptr);

    // Add some allowed entries.
    LogLoadAttempt(LogType::kAllowed, kTestLogs[i].basename_hash,
                   kTestLogs[i].code_id_hash, kTestPaths[i].c_str());
  }

  uint32_t initial_log = 0;
  DrainLog(nullptr, 0, &initial_log);
  ASSERT_TRUE(initial_log);

  uint8_t* buffer = reinterpret_cast<uint8_t*>(new uint8_t[initial_log]);
  uint32_t remaining_log = 0;
  uint32_t bytes_written = DrainLog(buffer, initial_log, &remaining_log);
  EXPECT_EQ(bytes_written, initial_log);
  EXPECT_EQ(remaining_log, uint32_t{0});

  VerifyBuffer(buffer, bytes_written);
  delete[] buffer;

  DeinitLogsForTesting();
}

// Test notifications.
TEST(Whitelist, LogNotifications) {
  // Init.
  ASSERT_EQ(InitLogs(), LogStatus::kSuccess);

  uint32_t initial_log = 0;
  DrainLog(nullptr, 0, &initial_log);
  EXPECT_EQ(initial_log, uint32_t{0});

  // Create an event for log notifications.
  g_test_event = ::CreateEvent(nullptr, false, false, nullptr);
  ASSERT_TRUE(g_test_event);
  // Register the event.
  ASSERT_TRUE(RegisterLogNotification(g_test_event));

  uint32_t log_count_expected = arraysize(kTestLogs);
  // Fire off a thread to handle the notifications.
  HANDLE thread = ::CreateThread(nullptr, 0, &NotificationHandler,
                                 &log_count_expected, 0, nullptr);

  for (size_t i = 0; i < log_count_expected; ++i) {
    // Add blocked entries - type doesn't matter in this test.
    LogLoadAttempt(LogType::kBlocked, kTestLogs[i].basename_hash,
                   kTestLogs[i].code_id_hash, nullptr);
  }

  EXPECT_EQ(::WaitForSingleObject(thread, kWaitTimeoutMs), WAIT_OBJECT_0);
  DWORD exit_code = 1;
  EXPECT_TRUE(::GetExitCodeThread(thread, &exit_code));
  EXPECT_EQ(exit_code, DWORD{0});

  DeinitLogsForTesting();
}

}  // namespace
}  // namespace whitelist
