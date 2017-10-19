// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WORKER_INTERFACE_BINDERS_H_
#define CONTENT_BROWSER_WORKER_INTERFACE_BINDERS_H_

#include "mojo/public/cpp/system/message_pipe.h"
#include "url/origin.h"

namespace content {
class RenderProcessHost;

void BindWorkerInterface(const std::string& interface_name,
                         mojo::ScopedMessagePipeHandle interface_pipe,
                         RenderProcessHost* host,
                         const url::Origin& origin);

}  // namespace content

#endif  // CONTENT_BROWSER_WORKER_INTERFACE_BINDERS_H_
