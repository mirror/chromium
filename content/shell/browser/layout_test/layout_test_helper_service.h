// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "third_party/WebKit/public/platform/layout_test_helper.mojom.h"

namespace content {

class LayoutTestHelperService : public service_manager::Service,
                                public blink::mojom::LayoutTestHelper {
 public:
  LayoutTestHelperService();
  ~LayoutTestHelperService() override;

  static std::unique_ptr<service_manager::Service> Create();

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // blink::mojom::LayoutTestHelper:
  void Reflect(const std::string& message, ReflectCallback callback) override;

  mojo::BindingSet<blink::mojom::LayoutTestHelper> bindings_;

  DISALLOW_COPY_AND_ASSIGN(LayoutTestHelperService);
};

}  // namespace content
