// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "services/video_capture/example_plugin/connection_dispatcher.h"
#include "services/video_capture/example_plugin/connection_handler.h"

int main(int argc, char** argv) {
  base::MessageLoop message_loop;

  video_capture::example_plugin::ConnectionHandler handler;
  video_capture::example_plugin::ConnectionDispatcher dispatcher(
      base::BindRepeating(&video_capture::example_plugin::ConnectionHandler::
                              OnVideoCaptureServiceConnected,
                          base::Unretained(&handler)));
  dispatcher.Init();
  dispatcher.RunConnectionAttemptLoop();

  // Run forever!
  base::RunLoop().Run();

  return 0;
}
