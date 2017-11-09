// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_SERVICE_H_
#define CHROME_BROWSER_CHROME_SERVICE_H_

#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/embedder/embedded_service_info.h"

namespace service_manager {
class Connector;
class Service;
}  // namespace service_manager

class ChromeBrowserMainExtraParts;

class ChromeService {
 public:
  ChromeService();
  ~ChromeService();

  // ChromeBrowserMain takes ownership of the returned parts.
  ChromeBrowserMainExtraParts* CreateExtraParts();

  service_manager::EmbeddedServiceInfo::ServiceFactory
  CreateChromeServiceFactory();

  service_manager::Connector* connector() { return connector_.get(); }

 private:
  class ExtraParts;
  class IOThreadContext;

  void InitConnector();

  std::unique_ptr<service_manager::Service> CreateChromeServiceWrapper();

  std::unique_ptr<service_manager::Connector> connector_;

  std::unique_ptr<IOThreadContext> io_thread_context_;

  DISALLOW_COPY_AND_ASSIGN(ChromeService);
};

#endif  // CHROME_BROWSER_CHROME_SERVICE_H_
