// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_COMMON_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_COMMON_H_

namespace content {
namespace service_worker {

const char kNoDocumentURLErrorMessage[] =
    "No URL is associated with the caller's document.";
const char kShutdownErrorMessage[] = "The Service Worker system has shutdown.";
const char kUserDeniedPermissionMessage[] =
    "The user denied permission to use Service Worker.";

}  // namespace service_worker
}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_COMMON_H_
