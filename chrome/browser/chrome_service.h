// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/launchable.h"

class ChromeService : public service_manager::Service {
 public:
  ChromeService();
  ~ChromeService() override = default;

  static std::unique_ptr<service_manager::Service> Create();

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& remote_info,
                       const std::string& name,
                       mojo::ScopedMessagePipeHandle handle) override;

  service_manager::BinderRegistry registry_;

#if defined(OS_CHROMEOS)
  chromeos::Launchable launchable_;
#endif
#if defined(USE_OZONE)
  ui::InputDeviceController input_device_controller_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ChromeService);
};
