// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_COM_BASE_UTIL_H_
#define BASE_WIN_COM_BASE_UTIL_H_

#include <hstring.h>
#include <inspectable.h>
#include <roapi.h>
#include <windef.h>

#include "base/base_export.h"

namespace base {
namespace win {

// Provides access to functions in combase.dll which may not be available on
// Windows 7. Loads functions dynamically at runtime to prevent library
// dependencies.
namespace com_base_util {

BASE_EXPORT bool PreloadRequiredFunctions();

BASE_EXPORT HRESULT RoGetActivationFactory(HSTRING class_id,
                                           const IID& iid,
                                           void** out_factory);

BASE_EXPORT HRESULT RoActivateInstance(HSTRING class_id,
                                       IInspectable** instance);

}  // namespace com_base_util
}  // namespace win
}  // namespace base

#endif  // BASE_WIN_COM_BASE_UTIL_H_
