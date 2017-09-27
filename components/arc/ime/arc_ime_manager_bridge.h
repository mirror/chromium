// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_IME_ARC_IME_MANAGER_BRIDGE_H_
#define COMPONENTS_ARC_IME_ARC_IME_MANAGER_BRIDGE_H_

#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/arc/common/ime.mojom.h"
#include "components/arc/instance_holder.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/base/ime/chromeos/input_method_descriptor.h"

class Profile;

namespace chromeos {
class InputMethodEngine;
}  // namespace chromeos

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

class ArcImeManagerBridge
    : public KeyedService,
      public InstanceHolder<mojom::ImeManagerInstance>::Observer,
      public mojom::ImeManagerHost {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcImeManagerBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcImeManagerBridge(content::BrowserContext* context,
                      ArcBridgeService* bridge_service);
  ~ArcImeManagerBridge() override;

  // InstanceHolder<mojom::ImeManagerInstance>::Observer overrides.
  void OnInstanceReady() override;

  // mojom::ImeManagerHost overrides.
  void NotifyDefaultImeChanged(const std::string& ime_id,
                               const std::string& ime_subtype_id) override;
  void NotifyEnabledImeInfo(std::vector<mojom::ImeInfoPtr> ime_infos) override;

 private:
  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  mojo::Binding<mojom::ImeManagerHost> binding_;
  Profile* const profile_;

  std::string current_arc_ime_id_;
  std::string current_arc_ime_subtype_id_;

  std::string arc_proxy_ime_id_;
  std::unique_ptr<chromeos::InputMethodEngine> ime_engine_;
  chromeos::input_method::InputMethodDescriptors ime_descriptors_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ArcImeManagerBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_IME_ARC_IME_MANAGER_BRIDGE_H_
