// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_SPAWNED_TEST_SERVER_REMOTE_TEST_SERVER_SPAWNER_REQUEST_H_
#define NET_TEST_SPAWNED_TEST_SERVER_REMOTE_TEST_SERVER_SPAWNER_REQUEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"

class GURL;

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace net {

// RemoteTestServerSpawnerRequest is used by RemoteTestServer to send a request
// to the test server spawner.
class RemoteTestServerSpawnerRequest {
 public:
  // Queries the specified URL. If |post_data| is empty then a GET request is
  // sent. Otherwise |post_data| must be a json blob which is sent as a POST
  // request body.
  RemoteTestServerSpawnerRequest(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      const GURL& url,
      const std::string& post_data);
  ~RemoteTestServerSpawnerRequest();

  // Blocks until request is finished. If |response| isn't nullptr then server
  // response is copied to *response. Returns true if the request was completed
  // successfully.
  bool WaitResult(std::string* response) WARN_UNUSED_RESULT;

 private:
  class Core;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  std::unique_ptr<Core> core_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(RemoteTestServerSpawnerRequest);
};

}  // namespace net

#endif  // NET_TEST_SPAWNED_TEST_SERVER_REMOTE_TEST_SERVER_SPAWNER_REQUEST_H_
