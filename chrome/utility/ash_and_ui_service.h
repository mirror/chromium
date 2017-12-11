// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_ASH_AND_UI_SERVICE_H_
#define CHROME_UTILITY_ASH_AND_UI_SERVICE_H_

#include "base/macros.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

// AshAndUiService runs the mojo ash and ui services in a single process.
// It listens for incoming CreateService() requests and instantiates the actual
// service implementations as needed. Used in ash::Config::MASH only.
class AshAndUiService : public service_manager::Service {
 public:
  AshAndUiService();
  ~AshAndUiService() override;

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(AshAndUiService);
};

#endif  // CHROME_UTILITY_ASH_AND_UI_SERVICE_H_
