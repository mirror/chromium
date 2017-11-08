// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_CHROME_RENDERER_SERVICE_H_
#define CHROME_RENDERER_CHROME_RENDERER_SERVICE_H_

#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class ChromeRendererService : public service_manager::Service {
 public:
  explicit ChromeRendererService(service_manager::BinderRegistry* registry);
  ~ChromeRendererService() override;

  static std::unique_ptr<service_manager::Service> Create(
      service_manager::BinderRegistry* registry);

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& remote_info,
                       const std::string& name,
                       mojo::ScopedMessagePipeHandle handle) override;

  service_manager::BinderRegistry* registry_;

  DISALLOW_COPY_AND_ASSIGN(ChromeRendererService);
};

#endif  // CHROME_RENDERER_CHROME_RENDERER_SERVICE_H_
