// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace blink {

#define bool bool

bool FunctionReturningBool(char* input_data) {
  return input_data[0];
}

}  // namespace blink
