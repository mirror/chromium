// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/hack.h"

#include <unistd.h>

#include "base/logging.h"

namespace base {

volatile HackState g_hack_state = base::Starting;

void HACK_Reset() {
  g_hack_state = Starting;
}

void HACK_WaitUntil(HackState state) {
  while (g_hack_state != state) {
    LOG(ERROR) << "  HACK waiting until state=" << HackNames[state] << "("
               << state << ")";
    sleep(1);
  }
}

void HACK_AdvanceState(HackState expected, HackState to) {
  CHECK_EQ(g_hack_state, expected);
  CHECK_EQ(expected + 1, to);
  LOG(ERROR) << "  HACK setting state=" << HackNames[to] << "(" << to << ")";
  g_hack_state = to;
  LOG(ERROR) << "  HACK setting state=" << HackNames[to] << "(" << to << ")";
}

}
