// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_PROTECTED_BUFFER_SERVICE_HOST_H
#define CHROME_BROWSER_CHROMEOS_ARC_PROTECTED_BUFFER_SERVICE_HOST_H

#include "base/macro.h"
#include "components/arc/common/protected_buffer.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/bindig_set.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

class GpuArcProtectedBufferServiceHost : public KeyedService,
                                         public mojom::ProtectedBufferHost {
 public:
  // Return singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC++.
  static GpuArcProtectedBufferServiceHost* GetForBrowserContext(
      content::BrowserContext* context);

  GpuArcProtectedBufferServiceHost(content::BrowserContext* context,
                                   ArcBridgeService* bridge_service);
  ~GpuArcProtectedBufferServiceHost() override;

  void OnBootstrapProtectedBufferFactory(
      OnBootstrapProtectedBufferFactoryCallback callback) override;

 private:
  ArcBridgeService* const arc_bridge_service_;
  std::unique_ptr<mojom::ProtectedBufferFactory> protected_buffer_factory_;
  mojo::BindingSet<mojom::ProtectedBufferFactory>
      protected_buffer_factory_bindings_;

  DISALLOW_COPY_AND_ASSIGN(GpuArcProtectedBuffer);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_PROTECTED_BUFFER_SERVICE_HOST_H
