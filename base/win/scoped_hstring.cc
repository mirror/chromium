// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/scoped_hstring.h"

#include "base/logging.h"
#include "base/win/com_base_util.h"

namespace {

template <typename signature>
bool GetProcAddress(HMODULE module,
                    signature* function_signature,
                    const std::string& function_name) {
  if (!module)
    return false;
  *function_signature = reinterpret_cast<signature>(
      ::GetProcAddress(module, function_name.c_str()));
  if (!*function_signature)
    return false;
  return true;
}

decltype(&::WindowsCreateString) ResolveWindowsCreateString() {
  HMODULE combase_dll = base::win::ComBaseUtil::GetComBaseModuleHandle();
  decltype(&::WindowsCreateString) create_string_func = nullptr;
  if (!GetProcAddress<decltype(&::WindowsCreateString)>(
          combase_dll, &create_string_func, "WindowsCreateString"))
    return nullptr;
  return create_string_func;
}

decltype(&::WindowsDeleteString) ResolveWindowsDeleteString() {
  HMODULE combase_dll = base::win::ComBaseUtil::GetComBaseModuleHandle();
  decltype(&::WindowsDeleteString) delete_string_func = nullptr;
  if (!GetProcAddress<decltype(&::WindowsDeleteString)>(
          combase_dll, &delete_string_func, "WindowsDeleteString"))
    return nullptr;
  return delete_string_func;
}

decltype(&::WindowsGetStringRawBuffer) ResolveWindowsGetStringRawBuffer() {
  HMODULE combase_dll = base::win::ComBaseUtil::GetComBaseModuleHandle();
  decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func = nullptr;
  if (!GetProcAddress<decltype(&::WindowsGetStringRawBuffer)>(
          combase_dll, &get_string_raw_buffer_func,
          "WindowsGetStringRawBuffer"))
    return nullptr;
  return get_string_raw_buffer_func;
}

decltype(&::WindowsCreateString) PreloadWindowsCreateString() {
  static decltype(&::WindowsCreateString) create_string_func =
      ResolveWindowsCreateString();
  return create_string_func;
}

decltype(&::WindowsDeleteString) PreloadWindowsDeleteString() {
  static decltype(&::WindowsDeleteString) delete_string_func =
      ResolveWindowsDeleteString();
  return delete_string_func;
}

decltype(&::WindowsGetStringRawBuffer) PreloadWindowsGetStringRawBuffer() {
  static decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func =
      ResolveWindowsGetStringRawBuffer();
  return get_string_raw_buffer_func;
}

}  // namespace

namespace base {
namespace win {

// static
HSTRING ScopedHStringTraits::InvalidValue() {
  return nullptr;
}

// static
void ScopedHStringTraits::Free(HSTRING hstr) {
  ScopedHString::WindowsDeleteString(hstr);
}

ScopedHString::ScopedHString(const base::char16* str) : ScopedGeneric(nullptr) {
  HSTRING hstr;
  HRESULT hr =
      WindowsCreateString(str, static_cast<uint32_t>(wcslen(str)), &hstr);
  if (FAILED(hr))
    VLOG(1) << "WindowsCreateString failed";
  else
    reset(hstr);
}

// static
bool ScopedHString::PreloadRequiredFunctions() {
  return PreloadWindowsCreateString() && PreloadWindowsDeleteString() &&
         PreloadWindowsGetStringRawBuffer();
}

HRESULT ScopedHString::WindowsCreateString(const base::char16* src,
                                           uint32_t len,
                                           HSTRING* out_hstr) {
  decltype(&::WindowsCreateString) create_string_func_ =
      PreloadWindowsCreateString();
  if (!create_string_func_)
    return E_ACCESSDENIED;
  return create_string_func_(src, len, out_hstr);
}

HRESULT ScopedHString::WindowsDeleteString(HSTRING hstr) {
  decltype(&::WindowsDeleteString) delete_string_func =
      PreloadWindowsDeleteString();
  if (!delete_string_func)
    return E_ACCESSDENIED;
  return delete_string_func(hstr);
}

const base::char16* ScopedHString::WindowsGetStringRawBuffer(
    HSTRING hstr,
    uint32_t* out_len) {
  decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func =
      PreloadWindowsGetStringRawBuffer();
  if (!get_string_raw_buffer_func)
    return L"";
  return get_string_raw_buffer_func(hstr, out_len);
}

}  // namespace win
}  // namespace base
