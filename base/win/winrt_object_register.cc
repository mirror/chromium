// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/winrt_object_register.h"

#include <wrl.h>

namespace mswr = Microsoft::WRL;

namespace base {
namespace win {

WinrtObjectRegister::WinrtObjectRegister() {}

WinrtObjectRegister::~WinrtObjectRegister() {
  if (has_registered_)
    UnregisterObjects();
  has_registered_ = false;
}

HRESULT WinrtObjectRegister::Run() {
  HRESULT hr = RegisterObjects();
  if (SUCCEEDED(hr))
    has_registered_ = true;
  return hr;
}

HRESULT WinrtObjectRegister::RegisterObjects() {
  mswr::Module<mswr::OutOfProc>::Create([] {});
  mswr::Module<mswr::OutOfProc>::GetModule().IncrementObjectCount();
  return mswr::Module<mswr::OutOfProc>::GetModule().RegisterObjects();
}

void WinrtObjectRegister::UnregisterObjects() {
  mswr::Module<mswr::OutOfProc>::GetModule().UnregisterObjects();
  mswr::Module<mswr::OutOfProc>::GetModule().DecrementObjectCount();
}

}  // namespace win
}  // namespace base