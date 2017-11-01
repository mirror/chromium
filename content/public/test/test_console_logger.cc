// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_console_logger.h"

namespace content {

TestConsoleLogger::TestConsoleLogger() = default;
TestConsoleLogger::~TestConsoleLogger() = default;

void TestConsoleLogger::LogInFrame(content::RenderFrameHost* render_frame_host,
                                   content::ConsoleMessageLevel level,
                                   const std::string& message) {
  messages_.push_back(message);
}

}  // namespace content
