// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_SERVICE_H_
#define CHROME_BROWSER_CHROME_SERVICE_H_

#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/embedder/embedded_service_info.h"

namespace service_manager {
class Identity;
class Service;
}  // namespace service_manager

class ChromeService {
 public:
  ChromeService();
  ~ChromeService();

  service_manager::EmbeddedServiceInfo::ServiceFactory
  CreateChromeServiceFactory();

  void BindInterface(const service_manager::Identity& identity,
                     const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe);

 private:
  class IOThreadContext;

  std::unique_ptr<service_manager::Service> CreateChromeServiceWrapper();

  std::unique_ptr<IOThreadContext> io_thread_context_;

  DISALLOW_COPY_AND_ASSIGN(ChromeService);
};

#endif  // CHROME_BROWSER_CHROME_SERVICE_H_
