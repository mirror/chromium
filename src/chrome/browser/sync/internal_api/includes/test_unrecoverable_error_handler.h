// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_INTERNAL_API_INCLUDES_TEST_UNRECOVERABLE_ERROR_HANDLER_H_
#define CHROME_BROWSER_SYNC_INTERNAL_API_INCLUDES_TEST_UNRECOVERABLE_ERROR_HANDLER_H_
#pragma once

#include "base/compiler_specific.h"
#include "chrome/browser/sync/internal_api/includes/unrecoverable_error_handler.h"

namespace browser_sync {

// Implementation of UnrecoverableErrorHandler that simply adds a
// gtest failure.
class TestUnrecoverableErrorHandler : public UnrecoverableErrorHandler {
 public:
  TestUnrecoverableErrorHandler();
  virtual ~TestUnrecoverableErrorHandler();

  virtual void OnUnrecoverableError(const tracked_objects::Location& from_here,
                                    const std::string& message) OVERRIDE;
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_INTERNAL_API_INCLUDES_TEST_UNRECOVERABLE_ERROR_HANDLER_H_

