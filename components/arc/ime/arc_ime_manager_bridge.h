// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_IME_ARC_IME_MANAGER_BRIDGE_H_
#define COMPONENTS_ARC_IME_ARC_IME_MANAGER_BRIDGE_H_

#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/arc/common/ime.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/base/ime/chromeos/input_method_descriptor.h"
#include "ui/base/ime/chromeos/input_method_manager.h"

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
      public mojom::ImeManagerHost,
      public chromeos::input_method::InputMethodManager::Observer {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcImeManagerBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcImeManagerBridge(content::BrowserContext* context,
                      ArcBridgeService* bridge_service);
  ~ArcImeManagerBridge() override;

  // mojom::ImeManagerHost overrides.
  void NotifyDefaultImeChanged(const std::string& ime_id,
                               const std::string& ime_subtype_id) override;
  void NotifyImeInfos(std::vector<mojom::ImeInfoPtr> ime_infos) override;

  // mojom::ImeManagerInstance delegates.
  void SwitchImeTo(const std::string& ime_id,
                   const std::string& ime_subtype_id);
  void EnableIme(const std::string& ime_id,
                 const std::string& ime_subtype_id,
                 bool enable);

  // chromeos::input_method::InputMethodManager::Obserber overrides.
  void InputMethodChanged(chromeos::input_method::InputMethodManager* manager,
                          Profile* profile,
                          bool show_message) override {}
  void ActiveInputMethodsChanged(
      chromeos::input_method::InputMethodManager* manager) override;

 private:
  class ArcImeProxyObserver;

  chromeos::input_method::InputMethodDescriptor buildImeDescriptor(
      const mojom::ImeInfo*,
      const mojom::ImeSubtypeInfo*) const;
  void EnableIme(const std::string& input_method_id, bool enable);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.
  Profile* const profile_;

  std::string current_arc_ime_id_;
  std::string current_arc_ime_subtype_id_;

  std::string proxy_ime_extension_id_;
  std::unique_ptr<chromeos::InputMethodEngine> proxy_ime_engine_;
  ArcImeProxyObserver* proxy_ime_engine_observer_;
  chromeos::input_method::InputMethodDescriptors proxy_ime_descriptors_;
  std::set<std::string> proxy_ime_ids_;
  std::set<std::string> enabled_proxy_ime_ids_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ArcImeManagerBridge);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_IME_ARC_IME_MANAGER_BRIDGE_H_
