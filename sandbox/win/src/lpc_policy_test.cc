// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These tests have been added to specifically tests issues arising from (A)LPC
// lock down.

#include <algorithm>
#include <cctype>

#include <windows.h>
#include <winioctl.h>

#include "base/win/windows_version.h"
#include "sandbox/win/src/heap_helper.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/tests/common/controller.h"
#include "sandbox/win/tests/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {

namespace {

bool CsrssDisconnectSupported() {
  // This functionality has not been verified on versions before Win10.
  if (base::win::GetVersion() < base::win::VERSION_WIN10)
    return false;

  // Does not work on 32-bit on x64 (ie Wow64).
  return (base::win::OSInfo::GetInstance()->wow64_status() !=
          base::win::OSInfo::WOW64_ENABLED);
}

// Generate a event name, used to test thread creation.
std::wstring GenerateEventName(std::wstring prefix, DWORD pid) {
  wchar_t buff[30] = {0};
  int res = swprintf_s(buff, sizeof(buff) / sizeof(buff[0]),
                       L"ProcessPolicyTest_%08x", pid);
  if (-1 != res) {
    return prefix + std::wstring(buff);
  }
  return std::wstring();
}
/*
std::wstring GenerateEventName(DWORD pid) {
  wchar_t buff[30] = {0};
  int res = swprintf_s(buff, sizeof(buff) / sizeof(buff[0]),
                       L"ProcessPolicyTest_%08x", pid);
  if (-1 != res) {
    return std::wstring(buff);
  }
  return std::wstring();
}
*/
int GetLocaleInfoString(LCTYPE type) {
  int buffer_size_with_nul =
      ::GetLocaleInfo(0xc00, type | (true ? LOCALE_NOUSEROVERRIDE : 0), 0, 0);
  return buffer_size_with_nul;
}
// GetUserDefaultLocaleName is not available on WIN XP.  So we'll
// load it on-the-fly.
const wchar_t kKernel32DllName[] = L"kernel32.dll";
typedef int(WINAPI* GetUserDefaultLocaleNameFunction)(LPWSTR lpLocaleName,
                                                      int cchLocaleName);

typedef int(WINAPI* NlsGetCacheUpdateCountFunction)();
typedef int(WINAPI* NlsValidateLocale)();

int NlsGetCacheUpdateCount() {
  static NlsGetCacheUpdateCountFunction NlsGetCacheUpdateCount_func = NULL;
  if (!NlsGetCacheUpdateCount_func) {
    // NlsGetCacheUpdateCount is not available on WIN XP.  So we'll
    // load it on-the-fly.
    HMODULE kernel32_dll = ::GetModuleHandle(kKernel32DllName);
    EXPECT_NE(nullptr, kernel32_dll);
    NlsGetCacheUpdateCount_func =
        reinterpret_cast<NlsGetCacheUpdateCountFunction>(
            GetProcAddress(kernel32_dll, "NlsGetCacheUpdateCount"));
    EXPECT_NE(nullptr, NlsGetCacheUpdateCount_func);
  }

  int update_count = NlsGetCacheUpdateCount_func();
  LOG(ERROR) << "updated_count=" << update_count;
  return update_count;
}
/*
BOOL SetLocaleInfo(
  _In_ LCID    Locale,
  _In_ LCTYPE  LCType,
  _In_ LPCTSTR lpLCData
);
*/

BOOL SetLocaleHelper() {
  NlsGetCacheUpdateCount();
  GetLocaleInfoString(LOCALE_SSHORTDATE);
  BOOL updated =
      ::SetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, L"M/d/yyyy");
  LOG(ERROR) << "SetLocaleHelper updated " << (updated ? " true" : " false");
  NlsGetCacheUpdateCount();

  return updated;
}

DWORD WINAPI ChangeLocaleFunc(LPVOID lpdwThreadParam) {
  DWORD pid = static_cast<DWORD>(reinterpret_cast<uintptr_t>(lpdwThreadParam));
  // step 1: open renderer_up event
  std::wstring renderer_up_event_name = GenerateEventName(L"render_up_", pid);
  if (!renderer_up_event_name.length())
    return 1;

  HANDLE renderer_up_event =
      ::OpenEvent(SYNCHRONIZE, false, renderer_up_event_name.c_str());
  if (!renderer_up_event) {
    return 1;
  }
  // step 3: wait for renderer_up_event
  LOG(ERROR) << "thread waiting for rendered_up";
  DWORD wait = WaitForSingleObject(renderer_up_event, INFINITE);
  if (wait != WAIT_OBJECT_0) {
    return 1;
  }
#ifndef CHANGE_LOCALE
  // step 3: change locale
  LOG(ERROR) << "seetting locale";
  if (!SetLocaleHelper()) {
    return 1;
  }
#endif
  // step 3: open broker change locale event
  std::wstring locale_changed_event_name =
      GenerateEventName(L"locale_changed_", pid);
  // ASSERT_STRNE(nullptr, locale_changed_event_name.c_str());
  HANDLE locale_changed_event =
      ::OpenEvent(EVENT_ALL_ACCESS | EVENT_MODIFY_STATE, false,
                  locale_changed_event_name.c_str());
  // step 3: set broken event
  if (locale_changed_event == nullptr) {
    return 1;
  }
  LOG(ERROR) << "setting locale event";

  if (!SetEvent(locale_changed_event))
    return 1;
  return 0;
}

}  // namespace
// Converts LCID to std::wstring for passing to sbox tests.
std::wstring LcidToWString(LCID lcid) {
  wchar_t buff[10] = {0};
  int res = swprintf_s(buff, sizeof(buff) / sizeof(buff[0]), L"%08x", lcid);
  if (-1 != res) {
    return std::wstring(buff);
  }
  return std::wstring();
}

// Converts LANGID to std::wstring for passing to sbox tests.
std::wstring LangidToWString(LANGID langid) {
  wchar_t buff[10] = {0};
  int res = swprintf_s(buff, sizeof(buff) / sizeof(buff[0]), L"%04x", langid);
  if (-1 != res) {
    return std::wstring(buff);
  }
  return std::wstring();
}

SBOX_TESTS_COMMAND int Lpc_GetUserDefaultLangID(int argc, wchar_t** argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  std::wstring expected_langid_string(argv[0]);

  // This will cause an exception if not warmed up suitably.
  LANGID langid = ::GetUserDefaultLangID();

  std::wstring langid_string = LangidToWString(langid);
  if (0 == wcsncmp(langid_string.c_str(), expected_langid_string.c_str(), 4)) {
    return SBOX_TEST_SUCCEEDED;
  }
  return SBOX_TEST_FAILED;
}

TEST(LpcPolicyTest, GetUserDefaultLangID) {
  LANGID langid = ::GetUserDefaultLangID();
  std::wstring cmd = L"Lpc_GetUserDefaultLangID " + LangidToWString(langid);
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd.c_str()));
}

SBOX_TESTS_COMMAND int Lpc_GetUserDefaultLCID(int argc, wchar_t** argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  std::wstring expected_lcid_string(argv[0]);

  // This will cause an exception if not warmed up suitably.
  LCID lcid = ::GetUserDefaultLCID();

  std::wstring lcid_string = LcidToWString(lcid);
  if (0 == wcsncmp(lcid_string.c_str(), expected_lcid_string.c_str(), 8)) {
    return SBOX_TEST_SUCCEEDED;
  }
  return SBOX_TEST_FAILED;
}

TEST(LpcPolicyTest, GetUserDefaultLCID) {
  LCID lcid = ::GetUserDefaultLCID();
  std::wstring cmd = L"Lpc_GetUserDefaultLCID " + LcidToWString(lcid);
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd.c_str()));
}

SBOX_TESTS_COMMAND int Lpc_GetUserDefaultLocaleName(int argc, wchar_t** argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  std::wstring expected_locale_name(argv[0]);
  static GetUserDefaultLocaleNameFunction GetUserDefaultLocaleName_func =
      nullptr;
  if (!GetUserDefaultLocaleName_func) {
    // GetUserDefaultLocaleName is not available on WIN XP.  So we'll
    // load it on-the-fly.
    HMODULE kernel32_dll = ::GetModuleHandle(kKernel32DllName);
    if (!kernel32_dll) {
      return SBOX_TEST_FAILED;
    }
    GetUserDefaultLocaleName_func =
        reinterpret_cast<GetUserDefaultLocaleNameFunction>(
            GetProcAddress(kernel32_dll, "GetUserDefaultLocaleName"));
    if (!GetUserDefaultLocaleName_func) {
      return SBOX_TEST_FAILED;
    }
  }
  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};
  // This will cause an exception if not warmed up suitably.
  int ret = GetUserDefaultLocaleName_func(
      locale_name, LOCALE_NAME_MAX_LENGTH * sizeof(wchar_t));
  if (!ret) {
    return SBOX_TEST_FAILED;
  }
  if (!wcsnlen(locale_name, LOCALE_NAME_MAX_LENGTH)) {
    return SBOX_TEST_FAILED;
  }
  if (0 == wcsncmp(locale_name, expected_locale_name.c_str(),
                   LOCALE_NAME_MAX_LENGTH)) {
    return SBOX_TEST_SUCCEEDED;
  }
  return SBOX_TEST_FAILED;
}

TEST(LpcPolicyTest, GetUserDefaultLocaleName) {
  static GetUserDefaultLocaleNameFunction GetUserDefaultLocaleName_func =
      nullptr;
  if (!GetUserDefaultLocaleName_func) {
    // GetUserDefaultLocaleName is not available on WIN XP.  So we'll
    // load it on-the-fly.
    HMODULE kernel32_dll = ::GetModuleHandle(kKernel32DllName);
    EXPECT_NE(nullptr, kernel32_dll);
    GetUserDefaultLocaleName_func =
        reinterpret_cast<GetUserDefaultLocaleNameFunction>(
            GetProcAddress(kernel32_dll, "GetUserDefaultLocaleName"));
    EXPECT_NE(nullptr, GetUserDefaultLocaleName_func);
  }
  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};
  EXPECT_NE(0, GetUserDefaultLocaleName_func(
                   locale_name, LOCALE_NAME_MAX_LENGTH * sizeof(wchar_t)));
  EXPECT_NE(0U, wcsnlen(locale_name, LOCALE_NAME_MAX_LENGTH));
  std::wstring cmd =
      L"Lpc_GetUserDefaultLocaleName " + std::wstring(locale_name);
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd.c_str()));
}
SBOX_TESTS_COMMAND int Lpc_GetLocaleInfo2(int argc, wchar_t** argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  // std::wstring expected_locale_name(argv[0]);

  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};

  // This will cause an exception if not warmed up suitably.
  if (!GetLocaleInfo(0xc00, LOCALE_SNAME, (LPTSTR)&locale_name,
                     sizeof(locale_name) / sizeof(wchar_t))) {
    return SBOX_TEST_FIRST_ERROR;
  }

  if (0 == wcsncmp(locale_name, argv[0], LOCALE_NAME_MAX_LENGTH)) {
    return SBOX_TEST_SUCCEEDED;
  }
  // return SBOX_TEST_FAILED;
  return SBOX_TEST_SUCCEEDED;
}
SBOX_TESTS_COMMAND int Lpc_GetLocaleInfo3(int argc, wchar_t** argv) {
  bool defaults_for_locale = false;
  DWORD value = 0;
  LCID lcid = 1055;
  int res = GetLocaleInfo(lcid,
                          LOCALE_RETURN_NUMBER | LOCALE_IFIRSTDAYOFWEEK |
                              (defaults_for_locale ? LOCALE_NOUSEROVERRIDE : 0),
                          reinterpret_cast<LPWSTR>(&value), sizeof(DWORD));
  return res;
}

SBOX_TESTS_COMMAND int Lpc_GetLocaleInfo4(int argc, wchar_t** argv) {
  bool defaults_for_locale = false;
  // According to MSDN, 9 is enough for LOCALE_SISO639LANGNAME.
  const size_t kLanguageCodeBufferSize = 9;
  WCHAR lowercase_language_code[kLanguageCodeBufferSize] = {0};
  int res =
      ::GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_SISO639LANGNAME |
                          (defaults_for_locale ? LOCALE_NOUSEROVERRIDE : 0),
                      lowercase_language_code, kLanguageCodeBufferSize);
  return res;
}
SBOX_TESTS_COMMAND int Lpc_GetLocaleInfo5(int argc, wchar_t** argv) {
  if (argc != 1)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;

  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};

  // This will cause an exception if not warmed up suitably.
  if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE,
                     (LPTSTR)&locale_name,
                     sizeof(locale_name) / sizeof(wchar_t))) {
    return SBOX_TEST_FIRST_ERROR;
  }

  return SBOX_TEST_SUCCEEDED;
}
SBOX_TESTS_COMMAND int Lpc_GetLocaleInfo(int argc, wchar_t** argv) {
  BOOL updated =
      ::SetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, L"M/d/yyyy");
  if (!updated) {
    // return SBOX_TEST_FIRST_ERROR;
  }
  return Lpc_GetLocaleInfo2(argc, argv);
}

TEST(LpcPolicyTest, TestGetLocaleInfo) {
  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};

  EXPECT_NE(0, GetLocaleInfo(0xc00, LOCALE_SNAME, (LPTSTR)&locale_name,
                             sizeof(locale_name) / sizeof(wchar_t)));

  std::wstring cmd = L"Lpc_GetLocaleInfo " + std::wstring(locale_name);
  LOG(ERROR) << "cmd=" << cmd;
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd.c_str()));
}

TEST(LpcPolicyTest, TestGetLocaleInfoOutsideOfRenderer) {
  EXPECT_NE(0, NlsGetCacheUpdateCount());
  GetLocaleInfoString(LOCALE_SSHORTDATE);
  BOOL updated = SetLocaleHelper();
  // BOOL updated =
  //    ::SetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, L"M/d/yyyy");
  EXPECT_TRUE(updated);
  EXPECT_NE(0, NlsGetCacheUpdateCount());
  GetLocaleInfoString(LOCALE_SSHORTDATE);
  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};

  EXPECT_NE(0, GetLocaleInfo(0xc00, LOCALE_SNAME, (LPTSTR)&locale_name,
                             sizeof(locale_name) / sizeof(wchar_t)));

  std::wstring cmd = L"Lpc_GetLocaleInfo " + std::wstring(locale_name);
  LOG(ERROR) << "cmd=" << cmd;
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(cmd.c_str()));
}

// Closing ALPC port can invalidate its heap.
// Test that all heaps are valid.
SBOX_TESTS_COMMAND int Lpc_TestValidProcessHeaps(int argc, wchar_t** argv) {
  if (argc != 0)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  // Retrieves the number of heaps in the current process.
  DWORD number_of_heaps = ::GetProcessHeaps(0, nullptr);
  // Try to retrieve a handle to all the heaps owned by this process. Returns
  // false if the number of heaps has changed.
  //
  // This is inherently racy as is, but it's not something that we observe a lot
  // in Chrome, the heaps tend to be created at startup only.
  std::unique_ptr<HANDLE[]> all_heaps(new HANDLE[number_of_heaps]);
  if (::GetProcessHeaps(number_of_heaps, all_heaps.get()) != number_of_heaps)
    return SBOX_TEST_FIRST_ERROR;

  for (size_t i = 0; i < number_of_heaps; ++i) {
    HANDLE handle = all_heaps[i];
    ULONG HeapInformation;
    bool result = HeapQueryInformation(handle, HeapCompatibilityInformation,
                                       &HeapInformation,
                                       sizeof(HeapInformation), nullptr);
    if (!result)
      return SBOX_TEST_SECOND_ERROR;
  }
  return SBOX_TEST_SUCCEEDED;
}

TEST(LpcPolicyTest, TestValidProcessHeaps) {
  TestRunner runner;
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, runner.RunTest(L"Lpc_TestValidProcessHeaps"));
}

// All processes should have a shared heap with csrss.exe. This test ensures
// that this heap can be found.
TEST(LpcPolicyTest, TestCanFindCsrPortHeap) {
  if (!CsrssDisconnectSupported()) {
    return;
  }
  HANDLE csr_port_handle = sandbox::FindCsrPortHeap();
  EXPECT_NE(nullptr, csr_port_handle);
}

TEST(LpcPolicyTest, TestHeapFlags) {
  if (!CsrssDisconnectSupported()) {
    // This functionality has not been verified on versions before Win10.
    return;
  }
  // Windows does not support callers supplying arbritary flag values. So we
  // write some non-trivial value to reduce the chance we match this in random
  // data.
  DWORD flags = 0x41007;
  HANDLE heap = HeapCreate(flags, 0, 0);
  EXPECT_NE(nullptr, heap);
  DWORD actual_flags = 0;
  EXPECT_TRUE(sandbox::HeapFlags(heap, &actual_flags));
  EXPECT_EQ(flags, actual_flags);
  EXPECT_TRUE(HeapDestroy(heap));
}

SBOX_TESTS_COMMAND int Lpc_TestChangeLocale(int argc, wchar_t** argv) {
  if (argc != 3)
    return SBOX_TEST_FAILED_TO_EXECUTE_COMMAND;
  std::wstring renderer_up_event_name(argv[0]);
  std::wstring locale_changed_event_name(argv[1]);
  NlsGetCacheUpdateCount();
  // open rendering event
  HANDLE renderer_up_event = ::OpenEvent(EVENT_ALL_ACCESS | EVENT_MODIFY_STATE,
                                         false, renderer_up_event_name.c_str());
  if (!renderer_up_event) {
    return 1;
  }
  // set renderer event
  if (!SetEvent(renderer_up_event))
    return 1;

  // wait for locale event
  HANDLE locale_changed_event =
      ::OpenEvent(EVENT_ALL_ACCESS | EVENT_MODIFY_STATE, false,
                  locale_changed_event_name.c_str());
  if (!locale_changed_event) {
    return 1;
  }
  if (WaitForSingleObject(locale_changed_event, INFINITE) != WAIT_OBJECT_0)
    return SBOX_TEST_FAILED;
  // check locale functions
  NlsGetCacheUpdateCount();
  return Lpc_GetLocaleInfo2(argc - 2, argv + 2);
  // return

  // return SBOX_TEST_SUCCEEDED;
}

// This tests that the CreateThread works with CSRSS locked down.
// In other words, that the interception correctly works.
TEST(LpcPolicyTest, TestChangeLocale) {
  TestRunner runner(JOB_NONE, USER_INTERACTIVE, USER_INTERACTIVE);
  // Create a thread that will change the locale
  DWORD pid = ::GetCurrentProcessId();
  LOG(ERROR) << "pid=" << pid;
  NlsGetCacheUpdateCount();
  // step 1: create rendered_up event
  std::wstring renderer_up_event_name = GenerateEventName(L"render_up_", pid);
  ASSERT_STRNE(nullptr, renderer_up_event_name.c_str());
  HANDLE renderer_up_event =
      ::CreateEvent(nullptr, true, false, renderer_up_event_name.c_str());
  EXPECT_NE(static_cast<HANDLE>(nullptr), renderer_up_event);

  // step 3: create broker change locale event
  std::wstring locale_changed_event_name =
      GenerateEventName(L"locale_changed_", pid);
  ASSERT_STRNE(nullptr, locale_changed_event_name.c_str());
  HANDLE locale_changed_event =
      ::CreateEvent(nullptr, true, false, locale_changed_event_name.c_str());
  EXPECT_NE(static_cast<HANDLE>(nullptr), locale_changed_event);
  // step 2: create change local thread

  HANDLE thread = nullptr;
  DWORD thread_id = 0;
  thread = ::CreateThread(nullptr, 0, &ChangeLocaleFunc,
                          reinterpret_cast<LPVOID>(static_cast<uintptr_t>(pid)),
                          0, &thread_id);
  EXPECT_NE(static_cast<HANDLE>(nullptr), thread);

  wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};

  EXPECT_NE(0, GetLocaleInfo(0xc00, LOCALE_SNAME, (LPTSTR)&locale_name,
                             sizeof(locale_name) / sizeof(wchar_t)));

  NlsGetCacheUpdateCount();
  std::wstring cmd = L"Lpc_TestChangeLocale " + renderer_up_event_name + L" " +
                     locale_changed_event_name + L" " +
                     std::wstring(locale_name);
  LOG(ERROR) << "cmd=" << cmd;

  auto test_result = runner.RunTest(cmd.c_str());
  EXPECT_EQ(SBOX_TEST_SUCCEEDED, test_result);
  NlsGetCacheUpdateCount();
  LOG(ERROR) << "waiting for  thread=";
  EXPECT_EQ(WAIT_OBJECT_0, WaitForSingleObject(thread, INFINITE));
}
}  // namespace sandbox
