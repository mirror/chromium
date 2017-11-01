// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_JAVA_HANDLER_THREAD_FOR_TESTING_H_
#define BASE_ANDROID_JAVA_HANDLER_THREAD_FOR_TESTING_H_

#include <memory>

namespace base {
namespace android {

class JavaHandlerThread;

// Test-only helpers for working with JavaHandlerThread.
class JavaHandlerThreadHelpers {
 public:
  // Create the Java peer first and test that it works before connecting to the
  // native object.
  static std::unique_ptr<JavaHandlerThread> CreateJavaFirst();

  static void HandleJniExceptionAndAbort();

 private:
  JavaHandlerThreadHelpers() = default;
  ~JavaHandlerThreadHelpers() = default;
};

}  // namespace android
}  // namespace base

#endif  // BASE_ANDROID_JAVA_HANDLER_THREAD_FOR_TESTING_H_
