// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/scoped_hstring.h"

#include <delayimp.h>
#include <winstring.h>

#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"

namespace {

HMODULE GetComBaseModuleHandle() {
  static HMODULE combase_dll = ::LoadLibrary(L"combase.dll");
  return combase_dll;
}

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
  HMODULE combase_dll = GetComBaseModuleHandle();
  decltype(&::WindowsCreateString) create_string_func = nullptr;
  if (!GetProcAddress<decltype(&::WindowsCreateString)>(
          combase_dll, &create_string_func, "WindowsCreateString"))
    return nullptr;
  return create_string_func;
}

decltype(&::WindowsDeleteString) ResolveWindowsDeleteString() {
  HMODULE combase_dll = GetComBaseModuleHandle();
  decltype(&::WindowsDeleteString) delete_string_func = nullptr;
  if (!GetProcAddress<decltype(&::WindowsDeleteString)>(
          combase_dll, &delete_string_func, "WindowsDeleteString"))
    return nullptr;
  return delete_string_func;
}

decltype(&::WindowsGetStringRawBuffer) ResolveWindowsGetStringRawBuffer() {
  HMODULE combase_dll = GetComBaseModuleHandle();
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

namespace internal {

HRESULT WindowsCreateString(const base::char16* src,
                            uint32_t len,
                            HSTRING* out_hstr) {
  decltype(&::WindowsCreateString) create_string_func_ =
      PreloadWindowsCreateString();
  if (!create_string_func_)
    return E_FAIL;
  return create_string_func_(src, len, out_hstr);
}

HRESULT WindowsDeleteString(HSTRING hstr) {
  decltype(&::WindowsDeleteString) delete_string_func =
      PreloadWindowsDeleteString();
  if (!delete_string_func)
    return E_FAIL;
  return delete_string_func(hstr);
}

const base::char16* WindowsGetStringRawBuffer(HSTRING hstr, uint32_t* out_len) {
  decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func =
      PreloadWindowsGetStringRawBuffer();
  if (!get_string_raw_buffer_func)
    return nullptr;
  return get_string_raw_buffer_func(hstr, out_len);
}

// static
void ScopedHStringTraits::Free(HSTRING hstr) {
  internal::WindowsDeleteString(hstr);
}

}  // namespace internal

namespace win {

// static
bool ScopedHString::load_succeeded = false;

// static
ScopedHString ScopedHString::Create(StringPiece16 str) {
  DCHECK(load_succeeded);
  HSTRING hstr;
  HRESULT hr = internal::WindowsCreateString(str.data(), str.length(), &hstr);
  if (SUCCEEDED(hr))
    return ScopedHString(hstr);
  DLOG(ERROR) << "Failed to create HSTRING" << std::hex << hr;
  return ScopedHString(nullptr);
}

ScopedHString ScopedHString::Create(StringPiece str) {
  return Create(UTF8ToWide(str));
}

ScopedHString::ScopedHString(HSTRING hstr) : ScopedGeneric(hstr) {
  DCHECK(load_succeeded);
}

// static
bool ScopedHString::ResolveCoreWinRTStringDelayload() {
  ThreadRestrictions::AssertIOAllowed();

  static const bool load_succeeded = []() {
    bool success = PreloadWindowsCreateString() &&
                   PreloadWindowsDeleteString() &&
                   PreloadWindowsGetStringRawBuffer();
    ScopedHString::load_succeeded = success;
    return success;
  }();
  return load_succeeded;
}

std::string ScopedHString::GetAsUTF8() const {
  std::string result;
  const StringPiece16 wide_string = Get();
  WideToUTF8(wide_string.data(), wide_string.length(), &result);
  return result;
}

const StringPiece16 ScopedHString::Get() const {
  UINT32 length = 0;
  const wchar_t* buffer = internal::WindowsGetStringRawBuffer(get(), &length);
  return StringPiece16(buffer, length);
}

}  // namespace win
}  // namespace base
