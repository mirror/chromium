// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_message_handler.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

class FileTransferMessageHandlerTest : public testing::Test {
 public:
  FileTransferMessageHandlerTest();
  ~FileTransferMessageHandlerTest() override;

  // testing::Test implementation.
  void SetUp() override;
  void TearDown() override;
};

FileTransferMessageHandlerTest::FileTransferMessageHandlerTest() = default;
FileTransferMessageHandlerTest::~FileTransferMessageHandlerTest() = default;

void FileTransferMessageHandlerTest::SetUp() {}

void FileTransferMessageHandlerTest::TearDown() {}

// TODO(jarhar): write unit tests

// TODO(jarhar): test for sending too many chunks

}  // namespace remoting
