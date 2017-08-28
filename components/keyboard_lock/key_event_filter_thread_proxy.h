// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_THREAD_PROXY_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_THREAD_PROXY_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "components/keyboard_lock/key_event_filter.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace keyboard_lock {

// TODO(zijiehe): Is this class necessary? i.e. Is it needed to forward
// RenderWidgetHost::ForwardKeyboardEvent() to network thread?
// An implementation of KeyEventFilter to forward all requests to |runner| and
// |filter|.
class KeyEventFilterThreadProxy : public KeyEventFilter {
 public:
  KeyEventFilterThreadProxy(
      scoped_refptr<base::SingleThreadTaskRunner> runner,
      std::unique_ptr<KeyEventFilter> filter);
  ~KeyEventFilterThreadProxy() override;

 private:
  // KeyEventFilter implementations
  // Because the requests will be sent to a different thread, these functions
  // always return true.
  bool OnKeyDown(ui::KeyboardCode code, int flags) override;
  bool OnKeyUp(ui::KeyboardCode code, int flags) override;

  const scoped_refptr<base::SingleThreadTaskRunner> runner_;
  const std::unique_ptr<KeyEventFilter> filter_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_THREAD_PROXY_H_
