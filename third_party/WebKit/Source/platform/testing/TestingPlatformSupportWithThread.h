// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TestingPlatformSupportWithThread_h
#define TestingPlatformSupportWithThread_h

#include <memory>
#include "platform/testing/TestingPlatformSupport.h"
#include "public/platform/WebThread.h"

namespace blink {

// This class adds mocked thread support to TestingPlatformSupport.  See also
// ScopedTestingPlatformSupport to use this class correctly.
class TestingPlatformSupportWithThread : public TestingPlatformSupport {
 public:
  TestingPlatformSupportWithThread();
  ~TestingPlatformSupportWithThread() override;

  // Platform:
  std::unique_ptr<WebThread> CreateThread(const char* name) override;

  // Works for created threads, but may not for the main thread. It relies on
  // Platform::Current()'s implementation.
  WebThread* CurrentThread() override;
};

}  // namespace blink

#endif  // TestingPlatformSupport_h
