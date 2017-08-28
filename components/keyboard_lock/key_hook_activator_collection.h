// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVATOR_COLLECTION_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVATOR_COLLECTION_H_

#include <map>
#include <memory>

#include "base/threading/thread_checker.h"
#include "components/keyboard_lock/key_hook_activator.h"

namespace content {
class WebContents;
}  // namespace content

namespace keyboard_lock {

class KeyHookActivator;

// A collection of KeyHookActivator instances.
// All the public functions can only be called in one thread.
class KeyHookActivatorCollection final {
 public:
  KeyHookActivatorCollection();
  ~KeyHookActivatorCollection();

  // Returning a nullptr if no KeyHookActivator has been registered for
  // |contents|.
  KeyHookActivator* Find(const content::WebContents* contents) const;

  void Insert(const content::WebContents* contents,
              std::unique_ptr<KeyHookActivator> key_hook);

  // Do nothing if |contents| is not found in the collection: web page may call
  // cancelKeyboardLock() without calling requestKeyboardLock() before.
  void Erase(const content::WebContents* contents);

 private:
  // GUARDED_BY(thread_checker_)
  std::map<const content::WebContents*, std::unique_ptr<KeyHookActivator>>
      activators_;

  // Ensures |activators_| is only accessed in |runner_| thread.
  base::ThreadChecker thread_checker_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVATOR_COLLECTION_H_
