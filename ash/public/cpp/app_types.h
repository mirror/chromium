// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_TYPES_H_
#define ASH_PUBLIC_CPP_APP_TYPES_H_

namespace ash {

// App type of the window.
// This enum is used to control a UMA histogram buckets and should be
// treated as append-only.
enum class AppType {
  OTHERS = 0,
  BROWSER,
  CHROME_APP,
  ARC_APP,
  APP_TYPE_LAST = ARC_APP,
};

const int kAppCount = static_cast<int>(AppType::APP_TYPE_LAST) + 1;

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_APP_TYPES_H_
