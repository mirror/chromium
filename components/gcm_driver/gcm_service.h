// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_MOJO_GCM_SERVICE_H_
#define COMPONENTS_GCM_DRIVER_MOJO_GCM_SERVICE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/gcm_driver/public/interfaces/gcm_receiver.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

class PrefService;

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace gcm {

class GCMDriver;

// Wrap it with GCMConnectionObserver
class GCMService : public service_manager::Service {
 public:
  GCMService();
  ~GCMService() override;

  void CreateGCMDriver(
      const GCMClient::ChromeBuildInfo& chrome_build_info,
      PrefService* pref_service,
      net::URLRequestContextGetter* url_context_getter,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& blocking_task_runner);

  GCMDriver* gcm_driver() const { return gcm_driver_.get(); }

 private:
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<GCMDriver> gcm_driver_;
  DISALLOW_COPY_AND_ASSIGN(GCMService);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_MOJO_GCM_SERVICE_H_
