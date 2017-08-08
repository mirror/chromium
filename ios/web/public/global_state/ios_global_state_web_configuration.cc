// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/global_state/ios_global_state_configuration.h"

#include "ios/web/public/web_thread.h"

namespace ios_global_state {

scoped_refptr<base::SingleThreadTaskRunner> GetIOThreadTaskRunner() {
  return web::WebThread::GetTaskRunnerForThread(web::WebThread::IO);
}

}  // namespace ios_global_state
