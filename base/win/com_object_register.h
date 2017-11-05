// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_COM_OBJECT_REGISTER_H_
#define BASE_WIN_COM_OBJECT_REGISTER_H_

#include "windows.h"

#include "base/base_export.h"
#include "base/threading/thread_checker.h"

namespace base {
namespace win {

class BASE_EXPORT ComObjectRegister {
 public:
  ComObjectRegister();
  ~ComObjectRegister();

  // Tries to register the COM object and sets the flag if succeeds.
  HRESULT Run();

 private:
  // Registers the COM object.
  HRESULT RegisterActivator();

  // Unregisters the COM object.
  void UnregisterActivator();

  // A flag indicating if the COM object is registered or not.
  bool has_registered_ = false;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ComObjectRegister);
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_COM_OBJECT_REGISTER_H_
