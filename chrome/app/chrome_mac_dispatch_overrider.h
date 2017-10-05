// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_CHROME_MACH_DISPATCH_OVERRIDER_H
#define CHROME_APP_CHROME_MACH_DISPATCH_OVERRIDER_H

// Sets the queue to return to CoreAudio's PauseIO and ResumeIO calls.
// MUST be called early enough there are no other threads/queues running.
extern "C" void chrome_dispatch_overrider_initialize();

#endif /* CHROME_APP_CHROME_MACH_DISPATCH_OVERRIDER_H */
