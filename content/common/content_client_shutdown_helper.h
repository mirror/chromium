// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/content_client.h"

namespace content {

class ContentClientShutdownHelper {
 public:
  template <class T>
  static void ContentClientPartDeleted(T* ptr) {
    T** storage = GetStorage<T>();
    if (storage && *storage == ptr)
      *storage = nullptr;
  }

 private:
  template <class T>
  static T** GetStorage();
};

}  // namespace content
