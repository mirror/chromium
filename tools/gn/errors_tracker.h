// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "tools/gn/err.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

class ErrorsTracker {
 public:
  void AddError(const Err& err);

  bool SerializeDiagnostics() const;

  static ErrorsTracker* GetInstance();

 private:
  ErrorsTracker();
  ~ErrorsTracker();

  friend struct base::DefaultSingletonTraits<ErrorsTracker>;

  std::vector<Err> errors_;

  DISALLOW_COPY_AND_ASSIGN(ErrorsTracker);
};
