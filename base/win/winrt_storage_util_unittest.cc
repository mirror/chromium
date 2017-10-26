// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/winrt_storage_util.h"

#include <string.h>
#include <windows.storage.streams.h>
#include <wrl/client.h>

#include "base/strings/string_util.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/scoped_hstring.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace win {

namespace {
const std::vector<uint8_t> KTestBufferData = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
}

TEST(WinrtStorageUtilTest, CreateBufferFromData) {
  ScopedCOMInitializer com_initializer(ScopedCOMInitializer::kMTA);

  if (!(ResolveCoreWinRTDelayload() &&
        ScopedHString::ResolveCoreWinRTStringDelayload()))
    return;

  Microsoft::WRL::ComPtr<ABI::Windows::Storage::Streams::IBuffer> buffer;
  ASSERT_HRESULT_SUCCEEDED(CreateIBufferFromData(
      KTestBufferData.data(), KTestBufferData.size(), buffer.GetAddressOf()));
  ASSERT_TRUE(buffer);

  uint8_t* p_buffer_data;
  ASSERT_HRESULT_SUCCEEDED(
      GetPointerToBufferData(buffer.Get(), &p_buffer_data));
  EXPECT_EQ(
      0, memcmp(p_buffer_data, KTestBufferData.data(), KTestBufferData.size()));
}

}  // namespace win
}  // namespace base
