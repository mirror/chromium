// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebIDBDatabaseInfo_h
#define WebIDBDatabaseInfo_h

#include "public/platform/WebString.h"

namespace blink {

struct WebIDBDatabaseInfo {
  enum { kNoVersion = -1 };

  WebString name;
  long long id;
  long long version;
};

}  // namespace blink

#endif  // WebIDBDatabaseInfo_h
