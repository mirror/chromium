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

decltype(&::RoGetActivationFactory) ResolveRoGetActivationFactory() {
  HMODULE combase_dll = base::win::ComBaseUtil::GetComBaseModuleHandle();
  decltype(&::RoGetActivationFactory) get_factory_func = nullptr;
  if (!GetProcAddress<decltype(&::RoGetActivationFactory)>(
          combase_dll, &get_factory_func, "RoGetActivationFactory"))
    return nullptr;
  return get_factory_func;
}

decltype(&::RoActivateInstance) ResolveRoActivateInstance() {
  HMODULE combase_dll = base::win::ComBaseUtil::GetComBaseModuleHandle();
  decltype(&::RoActivateInstance) activate_instance_func = nullptr;
  if (!GetProcAddress<decltype(&::RoActivateInstance)>(
          combase_dll, &activate_instance_func, "RoActivateInstance"))
    return nullptr;
  return activate_instance_func;
}

}  // namespace

namespace base {
namespace win {
namespace ComBaseUtil {

HMODULE GetComBaseModuleHandle() {
  static HMODULE combase_dll = ::LoadLibrary(L"combase.dll");
  return combase_dll;
}

decltype(&::RoGetActivationFactory) PreloadRoGetActivationFactory() {
  static decltype(&::RoGetActivationFactory) get_factory_func =
      ResolveRoGetActivationFactory();
  return get_factory_func;
}

decltype(&::RoActivateInstance) PreloadRoActivateInstance() {
  static decltype(&::RoActivateInstance) activate_instance_func =
      ResolveRoActivateInstance();
  return activate_instance_func;
}

HRESULT RoGetActivationFactory(HSTRING class_id,
                               const IID& iid,
                               void** out_factory) {
  decltype(&::RoGetActivationFactory) get_factory_func =
      PreloadRoGetActivationFactory();
  if (!get_factory_func)
    return E_ACCESSDENIED;
  return get_factory_func(class_id, iid, out_factory);
}

HRESULT RoActivateInstance(HSTRING class_id, IInspectable** instance) {
  static decltype(&::RoActivateInstance) activate_instance_func =
      PreloadRoActivateInstance();
  if (!activate_instance_func)
    return E_ACCESSDENIED;
  return activate_instance_func(class_id, instance);
}

}  // namespace ComBaseUtil
}  // namespace win
}  // namespace base
