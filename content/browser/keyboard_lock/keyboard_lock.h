// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_H_
#define CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_H_

namespace content {

// TODO: ADD COMMENTS.
// TODO: RENAME?  SINCE THIS IS A CONDUIT TO REACH A PLATFORMHOOK, UPDATE NAME
//       TO REFLECT THAT?
class KeyboardLock {
 public:
  virtual ~KeyboardLock() {};

  // Sets the reserved keys which are used for event filtering when KeyboardLock
  // is active.
  virtual void ReserveKeys() = 0;

  // Clears the set of keys which should be filtered.
  virtual void ClearReservedKeys() = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_KEYBOARD_LOCK_KEYBOARD_LOCK_H_
