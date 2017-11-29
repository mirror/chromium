// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/dialog_event_waiter.h"

#include <list>

template <typename DialogEvent>
DialogEventWaiter<DialogEvent>::DialogEventWaiter(
    std::list<DialogEvent> event_sequence)
    : events_(std::move(event_sequence)) {}

template <typename DialogEvent>
DialogEventWaiter<DialogEvent>::~DialogEventWaiter() {}

template <typename DialogEvent>
void DialogEventWaiter<DialogEvent>::Wait() {
  if (events_.empty())
    return;

  DCHECK(!run_loop_.running());
  run_loop_.Run();
}

template <typename DialogEvent>
void DialogEventWaiter<DialogEvent>::OnEvent(DialogEvent event) {
  if (events_.empty())
    return;

  ASSERT_EQ(events_.front(), event);
  events_.pop_front();
  // Only quit the loop if no other events are expected.
  if (events_.empty() && run_loop_.running())
    run_loop_.Quit();
}
