// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_event_filter_thread_proxy.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"

namespace keyboard_lock {

KeyEventFilterThreadProxy::KeyEventFilterThreadProxy(
    scoped_refptr<base::SingleThreadTaskRunner> runner,
    std::unique_ptr<KeyEventFilter> filter)
    : runner_(std::move(runner)),
      filter_(std::move(filter)) {
  DCHECK(runner_);
  DCHECK(filter_);
}

KeyEventFilterThreadProxy::~KeyEventFilterThreadProxy() {
  runner_->DeleteSoon(FROM_HERE,
      (*const_cast<std::unique_ptr<KeyEventFilter>*>(&filter_)).release());
}

bool KeyEventFilterThreadProxy::OnKeyDown(ui::KeyboardCode code, int flags) {
  runner_->PostTask(FROM_HERE, base::Bind(
      [](KeyEventFilter* filter, ui::KeyboardCode code, int flags) {
        filter->OnKeyDown(code, flags);
      },
      base::Unretained(filter_.get()),
      code,
      flags));
  return true;
}

bool KeyEventFilterThreadProxy::OnKeyUp(ui::KeyboardCode code, int flags) {
  runner_->PostTask(FROM_HERE, base::Bind(
      [](KeyEventFilter* filter, ui::KeyboardCode code, int flags) {
        filter->OnKeyUp(code, flags);
      },
      base::Unretained(filter_.get()),
      code,
      flags));
  return true;
}

}  // namespace keyboard_lock
