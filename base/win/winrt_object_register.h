// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_WINRT_OBJECT_REGISTER_H_
#define BASE_WIN_WINRT_OBJECT_REGISTER_H_

#include "windows.h"

#include "base/base_export.h"
#include "base/macros.h"

namespace base {
namespace win {

class BASE_EXPORT WinrtObjectRegister {
 public:
  WinrtObjectRegister();
  ~WinrtObjectRegister();

  // Registers the winrt object and sets the flag if succeeds.
  HRESULT Run();

  // Checks the registration status.
  bool HasRegistered() const { return has_registered_; }

 private:
  // Registers the winrt object.
  HRESULT RegisterWinrtObject();

  // Unregisters the winrt object.
  void UnregisterWinrtObject();

  // A flag indicating if the winrt object is registered or not.
  bool has_registered_ = false;

  DISALLOW_COPY_AND_ASSIGN(WinrtObjectRegister);
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_WINRT_OBJECT_REGISTER_H_
