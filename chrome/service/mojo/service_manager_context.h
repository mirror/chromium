// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICE_MOJO_SERVICE_MANAGER_CONTEXT_H_
#define CHROME_SERVICE_MOJO_SERVICE_MANAGER_CONTEXT_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace service_manager {
class Connector;
}

namespace content {
class ServiceManagerConnection;
}

// ServiceManagerContext manages the browser's connection to the ServiceManager,
// hosting a new in-process ServiceManagerContext if the browser was not
// launched from an external one.
class CONTENT_EXPORT ServiceManagerContext {
 public:
  explicit ServiceManagerContext(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~ServiceManagerContext();

  static service_manager::Connector* GetConnectorForIOThread();

 private:
  class InProcessServiceManagerContext;

  bool IsOnIOThread();

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  scoped_refptr<InProcessServiceManagerContext> in_process_context_;
  std::unique_ptr<content::ServiceManagerConnection>
      packaged_services_connection_;

  DISALLOW_COPY_AND_ASSIGN(ServiceManagerContext);
};

#endif  // CHROME_SERVICE_MOJO_SERVICE_MANAGER_CONTEXT_H_
