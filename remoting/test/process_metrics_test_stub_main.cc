// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrono>
#include <thread>

// This is a binary for tests of remoting::ProcessMetrics only. It sleeps
// forever.
int main(const int argv, const char* const argc[]) {
  std::this_thread::sleep_for(std::chrono::hours::max());
}
