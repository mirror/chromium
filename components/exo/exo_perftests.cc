// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "components/exo/test/exo_perftest.h"

void RectMain();

namespace exo {
namespace {

TEST_F(ExoPerfTest, Rect) {
  RunClient(base::Bind(RectMain));
}

}  // namespace
}  // namespace exo
