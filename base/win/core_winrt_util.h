// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_CORE_WINRT_UTIL_H_
#define BASE_WIN_CORE_WINRT_UTIL_H_

#include <hstring.h>
#include <inspectable.h>
#include <windef.h>
#include <windows.foundation.h>
#include <windows.storage.streams.h>

#include "base/base_export.h"
#include "base/strings/string_util.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_hstring.h"

namespace base {
namespace win {

// Provides access to Core WinRT functions which may not be available on
// Windows 7. Loads functions dynamically at runtime to prevent library
// dependencies.

BASE_EXPORT bool ResolveCoreWinRTDelayload();

// The following stubs are provided for when component build is enabled, in
// order to avoid the propogation of delay-loading CoreWinRT to other modules.

BASE_EXPORT HRESULT RoGetActivationFactory(HSTRING class_id,
                                           const IID& iid,
                                           void** out_factory);

BASE_EXPORT HRESULT RoActivateInstance(HSTRING class_id,
                                       IInspectable** instance);

BASE_EXPORT HRESULT
GetPointerToBufferData(ABI::Windows::Storage::Streams::IBuffer* buffer,
                       uint8_t** out);

BASE_EXPORT HRESULT CreateIBufferFromData(
    const uint8_t* data,
    const UINT32 length,
    ScopedComPtr<ABI::Windows::Storage::Streams::IBuffer>* buffer);

// Factory functions that activate and create WinRT components. The caller takes
// ownership of the returning ComPtr.
template <typename InterfaceType, base::char16 const* runtime_class_id>
ScopedComPtr<InterfaceType> WrlStaticsFactory() {
  ScopedHString class_id_hstring = ScopedHString::Create(runtime_class_id);
  if (!class_id_hstring.is_valid())
    return nullptr;

  ScopedComPtr<InterfaceType> com_ptr;
  HRESULT hr = base::win::RoGetActivationFactory(class_id_hstring.get(),
                                                 IID_PPV_ARGS(&com_ptr));
  if (FAILED(hr))
    return nullptr;

  return com_ptr;
}

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_CORE_WINRT_UTIL_H_
