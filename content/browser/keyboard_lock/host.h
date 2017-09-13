// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_HOST_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_HOST_H_

#include "ui/aura/keyboard_lock/host.h"

#include "build/build_config.h"
#if defined(OS_WIN)
#include "ui/aura/keyboard_lock/platform_hook_win.h"
#else
#include "ui/aura/keyboard_lock/platform_hook_null.h"
#endif

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

namespace content {

class WebContents;

namespace keyboard_lock {

#if defined(OS_WIN)
using PlatformKeyboardHookType = ui::aura::keyboard_lock::PlatformHookWin;
#else
using PlatformKeyboardHookType = ui::aura::keyboard_lock::PlatformHookNull;
#endif
using BaseHost = ui::aura::keyboard_lock::Host<content::WebContents*,
                                               PlatformKeyboardHookType>;

class Host final : public BaseHost {
 public:
  static Host* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<Host>;

  // This is a singleton object cross different Browser instances.
  Host();
  ~Host() override;

  using ObserverType = ui::aura::keyboard_lock::Observer<content::WebContents*>;
  std::unique_ptr<ui::aura::keyboard_lock::Client> CreateClient(
      content::WebContents* const& client) override;
  void ObserveClient(
      content::WebContents* const& client,
      std::unique_ptr<ObserverType> observer) override;
};

}  // namespace keyboard_lock
}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_HOST_H_
