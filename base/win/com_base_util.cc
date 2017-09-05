// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_base_util.h"

#include "base/logging.h"

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

}  // namespace

namespace base {
namespace win {

decltype(&::RoGetActivationFactory) ComBaseUtil::get_factory_func_ = nullptr;
decltype(&::RoActivateInstance) ComBaseUtil::activate_instance_func_ = nullptr;
decltype(&::WindowsCreateString) ComBaseUtil::create_string_func_ = nullptr;
decltype(&::WindowsDeleteString) ComBaseUtil::delete_string_func_ = nullptr;
decltype(&::WindowsCreateStringReference) ComBaseUtil::create_string_ref_func_ =
    nullptr;
decltype(
    &::WindowsGetStringRawBuffer) ComBaseUtil::get_string_raw_buffer_func_ =
    nullptr;

// static
bool ComBaseUtil::PreloadRoGetActivationFactory() {
  HMODULE combase_dll = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::RoGetActivationFactory)>(
      combase_dll, &get_factory_func_, "RoGetActivationFactory");
}

// static
bool ComBaseUtil::PreloadRoActivateInstance() {
  HMODULE combase_dll = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::RoActivateInstance)>(
      combase_dll, &activate_instance_func_, "RoActivateInstance");
}

// static
bool ComBaseUtil::PreloadWindowsCreateString() {
  HMODULE combase_dll = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsCreateString)>(
      combase_dll, &create_string_func_, "WindowsCreateString");
}

// static
bool ComBaseUtil::PreloadWindowsDeleteString() {
  HMODULE combase_dll = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsDeleteString)>(
      combase_dll, &delete_string_func_, "WindowsDeleteString");
}

// static
bool ComBaseUtil::PreloadWindowsCreateStringReference() {
  HMODULE combase_dll = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsCreateStringReference)>(
      combase_dll, &create_string_ref_func_, "WindowsCreateStringReference");
}

// static
bool ComBaseUtil::PreloadWindowsGetStringRawBuffer() {
  HMODULE combase_dll = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsGetStringRawBuffer)>(
      combase_dll, &get_string_raw_buffer_func_, "WindowsGetStringRawBuffer");
}

// static
bool ComBaseUtil::PreloadFunctions(const std::vector<std::string>& functions) {
  int function_count = functions.size();
  int functions_loaded = 0;

  std::string name("RoGetActivationFactory");
  if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
    if (!PreloadRoGetActivationFactory())
      return false;
    functions_loaded++;
  }

  name = "RoActivateInstance";
  if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
    if (!PreloadRoActivateInstance())
      return false;
    functions_loaded++;
  }

  name = "WindowsCreateString";
  if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
    if (!PreloadWindowsCreateString())
      return false;
    functions_loaded++;
  }

  name = "WindowsDeleteString";
  if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
    if (!PreloadWindowsDeleteString())
      return false;
    functions_loaded++;
  }

  name = "WindowsCreateStringReference";
  if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
    if (!PreloadWindowsCreateStringReference())
      return false;
    functions_loaded++;
  }

  name = "WindowsGetStringRawBuffer";
  if (std::find(functions.begin(), functions.end(), name) != functions.end()) {
    if (!PreloadWindowsGetStringRawBuffer())
      return false;
    functions_loaded++;
  }

  DCHECK_EQ(functions_loaded, function_count);
  return true;
}

// static
HRESULT ComBaseUtil::RoGetActivationFactory(HSTRING class_id,
                                            const IID& iid,
                                            void** out_factory) {
  if (!get_factory_func_ && !PreloadRoGetActivationFactory())
    return E_ACCESSDENIED;
  return get_factory_func_(class_id, iid, out_factory);
}

// static
HRESULT ComBaseUtil::RoActivateInstance(HSTRING class_id,
                                        IInspectable** instance) {
  if (!activate_instance_func_ && !PreloadRoActivateInstance())
    return E_ACCESSDENIED;
  return activate_instance_func_(class_id, instance);
}

// static
HRESULT ComBaseUtil::WindowsCreateString(const base::char16* src,
                                         uint32_t len,
                                         HSTRING* out_hstr) {
  if (!create_string_func_ && !PreloadWindowsCreateString())
    return E_ACCESSDENIED;
  return create_string_func_(src, len, out_hstr);
}

// static
HRESULT ComBaseUtil::WindowsDeleteString(HSTRING hstr) {
  if (!delete_string_func_ && !PreloadWindowsDeleteString())
    return E_ACCESSDENIED;
  return delete_string_func_(hstr);
}

// static
HRESULT ComBaseUtil::WindowsCreateStringReference(const base::char16* src,
                                                  uint32_t len,
                                                  HSTRING_HEADER* out_header,
                                                  HSTRING* out_hstr) {
  if (!create_string_ref_func_ && !PreloadWindowsCreateStringReference())
    return E_ACCESSDENIED;
  return create_string_ref_func_(src, len, out_header, out_hstr);
}

// static
const base::char16* ComBaseUtil::WindowsGetStringRawBuffer(HSTRING hstr,
                                                           uint32_t* out_len) {
  if (!get_string_raw_buffer_func_ && !PreloadWindowsGetStringRawBuffer())
    return L"";
  return get_string_raw_buffer_func_(hstr, out_len);
}

// static
HMODULE ComBaseUtil::EnsureLibraryLoaded() {
  static HMODULE combase_dll = nullptr;
  if (combase_dll == nullptr)
    combase_dll = ::LoadLibrary(L"combase.dll");
  return combase_dll;
}

}  // namespace win
}  // namespace base
