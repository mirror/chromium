// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_hook_activator_collection.h"

#include <utility>

namespace keyboard_lock {

KeyHookActivatorCollection::KeyHookActivatorCollection() = default;
KeyHookActivatorCollection::~KeyHookActivatorCollection() = default;

KeyHookActivator* KeyHookActivatorCollection::Find(
    const content::WebContents* contents) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  auto it = activators_.find(contents);
  if (it == activators_.end()) {
    return nullptr;
  }
  return it->second.get();
}

void KeyHookActivatorCollection::Insert(
    const content::WebContents* contents,
    std::unique_ptr<KeyHookActivator> key_hook) {
  DCHECK(thread_checker_.CalledOnValidThread());
  auto result = activators_.insert(
      std::make_pair(contents, std::move(key_hook)));
  DCHECK(result.second);
}

void KeyHookActivatorCollection::Erase(const content::WebContents* contents) {
  DCHECK(thread_checker_.CalledOnValidThread());
  activators_.erase(contents);
}

}  // namespace keyboard_lock
