// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/internal_api/includes/test_unrecoverable_error_handler.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace browser_sync {

TestUnrecoverableErrorHandler::TestUnrecoverableErrorHandler() {}

TestUnrecoverableErrorHandler::~TestUnrecoverableErrorHandler() {}

void TestUnrecoverableErrorHandler::OnUnrecoverableError(
    const tracked_objects::Location& from_here,
    const std::string& message) {
  ADD_FAILURE_AT(from_here.file_name(), from_here.line_number())
      << from_here.function_name() << ": " << message;
}

}  // namespace browser_sync

