// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HACK_H_
#define BASE_HACK_H_

namespace base {

#define HACK_STATES                      \
  X(Starting)                            \
  X(GoingToFlushWorkerPool)              \
  X(FlushedWorkerPool)                   \
  X(GoingToPostInit)                     \
  X(PostedInit)                          \
  X(AboutToCheck)                        \
  X(GoingToPostLoadIndexEntriesReply)    \
  X(PostedLoadIndexEntriesReply)         \
  X(NowChecking)

enum HackState {
#define X(n) n,
  HACK_STATES
#undef X
};

constexpr const char* const HackNames[] = {
#define X(n) #n,
    HACK_STATES
#undef X
};

void HACK_Reset();
void HACK_WaitUntil(HackState state);
void HACK_AdvanceState(HackState expected, HackState to);

}  // namespace base

#endif  // BASE_HACK_H_
