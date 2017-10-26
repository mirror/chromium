// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_WINRT_STORAGE_UTIL_H_
#define BASE_WIN_WINRT_STORAGE_UTIL_H_

#include <stdint.h>
#include <windows.h>

#include "base/base_export.h"

namespace ABI {
namespace Windows {
namespace Storage {
namespace Streams {
struct IBuffer;
}  // namespace Streams
}  // namespace Storage
}  // namespace Windows
}  // namespace ABI

namespace base {
namespace win {

BASE_EXPORT HRESULT
GetPointerToBufferData(ABI::Windows::Storage::Streams::IBuffer* buffer,
                       uint8_t** out);

BASE_EXPORT HRESULT
CreateIBufferFromData(const uint8_t* data,
                      UINT32 length,
                      ABI::Windows::Storage::Streams::IBuffer** buffer);

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_WINRT_STORAGE_UTIL_H_
