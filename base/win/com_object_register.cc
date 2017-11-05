// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/com_object_register.h"

#include <wrl.h>

namespace mswr = Microsoft::WRL;

namespace base {
namespace win {

ComObjectRegister::ComObjectRegister() {}

ComObjectRegister::~ComObjectRegister() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (has_registered_)
    UnregisterActivator();
  has_registered_ = false;
}

HRESULT ComObjectRegister::Run() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  HRESULT hr = RegisterActivator();
  if (SUCCEEDED(hr))
    has_registered_ = true;
  return hr;
}

HRESULT ComObjectRegister::RegisterActivator() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mswr::Module<mswr::OutOfProc>::Create([] {});
  mswr::Module<mswr::OutOfProc>::GetModule().IncrementObjectCount();
  return mswr::Module<mswr::OutOfProc>::GetModule().RegisterObjects();
}

void ComObjectRegister::UnregisterActivator() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mswr::Module<mswr::OutOfProc>::GetModule().UnregisterObjects();
  mswr::Module<mswr::OutOfProc>::GetModule().DecrementObjectCount();
}

}  // namespace win
}  // namespace base