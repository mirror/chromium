// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_base_util.h"

#include "base/logging.h"

namespace {

static HMODULE combase_dll_ = nullptr;

static decltype(&::RoGetActivationFactory) get_factory_func_ = nullptr;
static decltype(&::RoActivateInstance) activate_instance_func_ = nullptr;
static decltype(&::WindowsCreateString) create_string_func_ = nullptr;
static decltype(&::WindowsDeleteString) delete_string_func_ = nullptr;
static decltype(&::WindowsCreateStringReference) create_string_ref_func_ =
    nullptr;
static decltype(&::WindowsGetStringRawBuffer) get_string_raw_buffer_func_ =
    nullptr;

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
namespace ComBaseUtil {

HMODULE EnsureLibraryLoaded() {
  if (combase_dll_ == nullptr)
    combase_dll_ = ::LoadLibrary(L"combase.dll");
  return combase_dll_;
}

bool PreloadRoGetActivationFactory() {
  if (get_factory_func_)
    return true;
  HMODULE combase_dll_ = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::RoGetActivationFactory)>(
      combase_dll_, &get_factory_func_, "RoGetActivationFactory");
}

bool PreloadRoActivateInstance() {
  if (activate_instance_func_)
    return true;
  HMODULE combase_dll_ = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::RoActivateInstance)>(
      combase_dll_, &activate_instance_func_, "RoActivateInstance");
}

bool PreloadWindowsCreateString() {
  if (create_string_func_)
    return true;
  HMODULE combase_dll_ = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsCreateString)>(
      combase_dll_, &create_string_func_, "WindowsCreateString");
}

bool PreloadWindowsDeleteString() {
  if (delete_string_func_)
    return true;
  HMODULE combase_dll_ = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsDeleteString)>(
      combase_dll_, &delete_string_func_, "WindowsDeleteString");
}

bool PreloadWindowsGetStringRawBuffer() {
  if (get_string_raw_buffer_func_)
    return true;
  HMODULE combase_dll_ = EnsureLibraryLoaded();
  return GetProcAddress<decltype(&::WindowsGetStringRawBuffer)>(
      combase_dll_, &get_string_raw_buffer_func_, "WindowsGetStringRawBuffer");
}

HRESULT RoGetActivationFactory(HSTRING class_id,
                               const IID& iid,
                               void** out_factory) {
  if (!get_factory_func_ && !PreloadRoGetActivationFactory())
    return E_ACCESSDENIED;
  return get_factory_func_(class_id, iid, out_factory);
}

HRESULT RoActivateInstance(HSTRING class_id, IInspectable** instance) {
  if (!activate_instance_func_ && !PreloadRoActivateInstance())
    return E_ACCESSDENIED;
  return activate_instance_func_(class_id, instance);
}

HRESULT WindowsCreateString(const base::char16* src,
                            uint32_t len,
                            HSTRING* out_hstr) {
  if (!create_string_func_ && !PreloadWindowsCreateString())
    return E_ACCESSDENIED;
  return create_string_func_(src, len, out_hstr);
}

HRESULT WindowsDeleteString(HSTRING hstr) {
  if (!delete_string_func_ && !PreloadWindowsDeleteString())
    return E_ACCESSDENIED;
  return delete_string_func_(hstr);
}

const base::char16* WindowsGetStringRawBuffer(HSTRING hstr, uint32_t* out_len) {
  if (!get_string_raw_buffer_func_ && !PreloadWindowsGetStringRawBuffer())
    return L"";
  return get_string_raw_buffer_func_(hstr, out_len);
}

HMODULE GetLibraryForTesting() {
  return combase_dll_;
}

}  // namespace ComBaseUtil
}  // namespace win
}  // namespace base
