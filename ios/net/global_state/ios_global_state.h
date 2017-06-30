// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_GLOBAL_STATE_IOS_GLOBAL_STATE_H_
#define IOS_NET_GLOBAL_STATE_IOS_GLOBAL_STATE_H_

namespace ios_global_state {

// Creates global state for iOS. This should be called as early as possible in
// the application lifecycle. It is safe to call this method more than once, the
// initialization will only be performed once.
//
// An AtExitManger will only be created if |register_exit_manager| is true. If
// |register_exit_manager| is false, an AtExitManager must already exist before
// calling |Create|.
// |argc| and |argv| may be set to the command line options which were passed to
// the application.
//
// Since the initialization will only be performed the first time this method is
// called, the values of all the parameters will be ignored after the first
// call.
void Create(bool register_exit_manager, int argc, const char** argv);

// Creates a message loop for the UI thread and attaches it. It is safe to call
// this method more than once, the initialization will only be performed once.
void BuildMessageLoop();

// Starts a global base::TaskScheduler. This method must be called to start
// the Task Scheduler that is created in |Create|. It is safe to call this
// method more than once, the task scheduler will only be started once.
void StartTaskScheduler();

}  // namespace ios_global_state

#endif  // IOS_NET_GLOBAL_STATE_IOS_GLOBAL_STATE_H_
