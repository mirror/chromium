// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/ime/arc_ime_manager_bridge.h"

#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/input_method/input_method_engine.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "ui/base/ime/chromeos/input_method_util.h"
#include "url/gurl.h"

namespace arc {
namespace {

class ArcImeProxyObserver
    : public input_method::InputMethodEngineBase::Observer {
 public:
  ArcImeProxyObserver() {}
  ~ArcImeProxyObserver() {}

  void OnActivate(const std::string& engine_id) override {
    VLOG(1) << "@@@@@ OnActivate: engine_id=" << engine_id;
  }
  void OnFocus(
      const ui::IMEEngineHandlerInterface::InputContext& context) override {
    VLOG(1) << "@@@@@ OnFocus: context.id=" << context.id
            << " .type=" << context.type;
  }
  void OnBlur(int context_id) override {
    VLOG(1) << "@@@@@ OnBlur: context_id=" << context_id;
  }
  void OnKeyEvent(
      const std::string& engine_id,
      const input_method::InputMethodEngineBase::KeyboardEvent& event,
      ui::IMEEngineHandlerInterface::KeyEventDoneCallback& key_data) override {
    VLOG(1) << "@@@@@ OnKeyEvent: engine_id=" << engine_id
            << " event.type=" << event.type << " .key=" << event.key
            << " .code=" << event.code << ":" << event.key_code;
  }
  void OnReset(const std::string& engine_id) override {
    VLOG(1) << "@@@@@ OnReset: engine_id=" << engine_id;
  }
  void OnDeactivated(const std::string& engine_id) override {
    VLOG(1) << "@@@@@ OnDeactivated: engine_id=" << engine_id;
  }
  void OnCompositionBoundsChanged(
      const std::vector<gfx::Rect>& bounds) override {
    std::string x;
    for (const auto& rect : bounds) {
      if (x.length() > 0)
        x += "|";
      x += rect.ToString();
    }
    VLOG(1) << "@@@@@ OnCompositionBoundsChanged: bounds=" << x;
  }
  bool IsInterestedInKeyEvent() const override { return false; }
  void OnSurroundingTextChanged(const std::string& engine_id,
                                const std::string& text,
                                int cursor_pos,
                                int anchor_pos,
                                int offset_pos) override {
    VLOG(1) << "@@@@@ OnSurroundingTextChanged: engine_id=" << engine_id
            << " text=" << text << " cursor=" << cursor_pos
            << " anchor=" << anchor_pos << " offset=" << offset_pos;
  }
  void OnRequestEngineSwitch() override {
    VLOG(1) << "@@@@@ OnRequestEngineSwitch";
  }
  void OnInputContextUpdate(
      const ui::IMEEngineHandlerInterface::InputContext& context) override {
    VLOG(1) << "@@@@@ OnInputContextUpdate: context.id=" << context.id
            << " .type=" << context.type;
  }
  void OnCandidateClicked(
      const std::string& component_id,
      int candidate_id,
      input_method::InputMethodEngineBase::MouseButtonEvent button) override {
    VLOG(1) << "@@@@@ OnCandidateClicked: component_id=" << component_id
            << " candidate_id=" << candidate_id;
  }
  void OnMenuItemActivated(const std::string& component_id,
                           const std::string& menu_id) override {
    VLOG(1) << "@@@@@ OnMenuItemActivated: component_id=" << component_id
            << " menu_id=" << menu_id;
  }
};

// Singleton factory for ArcImeManagerBridge.
class ArcImeManagerBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcImeManagerBridge,
          ArcImeManagerBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcImeManagerBridgeFactory";

  static ArcImeManagerBridgeFactory* GetInstance() {
    return base::Singleton<ArcImeManagerBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcImeManagerBridgeFactory>;
  ArcImeManagerBridgeFactory() = default;
  ~ArcImeManagerBridgeFactory() override = default;
};

}  // namespace

// static
ArcImeManagerBridge* ArcImeManagerBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcImeManagerBridgeFactory::GetForBrowserContext(context);
}

ArcImeManagerBridge::ArcImeManagerBridge(content::BrowserContext* context,
                                         ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      profile_(Profile::FromBrowserContext(context)) {
  arc_bridge_service_->ime_manager()->AddObserver(this);
  VLOG(1) << "@@@@@ ArcImeManagerBridge created";
}

ArcImeManagerBridge::~ArcImeManagerBridge() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  arc_bridge_service_->ime_manager()->RemoveObserver(this);
  VLOG(1) << "@@@@@ ArcImeManagerBridge destroyed";
}

void ArcImeManagerBridge::OnInstanceReady() {
  VLOG(1) << "@@@@@ OnInstanceReady";
  mojom::ImeManagerInstance* ime_manager_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->ime_manager(), Init);
  DCHECK(ime_manager_instance);
  mojom::ImeManagerHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  ime_manager_instance->Init(std::move(host_proxy));
}

void ArcImeManagerBridge::NotifyDefaultImeChanged(
    const std::string& ime_id,
    const std::string& ime_subtype_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  VLOG(1) << "@@@@@ NotifyDefaultImeChanged: ime_id=" << ime_id
          << " ime_subtype_id=" << ime_subtype_id;
  current_arc_ime_id_ = ime_id;
  current_arc_ime_subtype_id_ = ime_subtype_id;
}

void ArcImeManagerBridge::NotifyEnabledImeInfo(
    std::vector<mojom::ImeInfoPtr> ime_infos) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // TODO: Create proxy IMEs' descriptors.
  VLOG(1) << "@@@@@ NotifyEnabledImeInfo: ";
  for (const auto& ime_info : ime_infos) {
    VLOG(1) << "@@@@@     id=" << ime_info->id
            << " displayName=" << ime_info->displayName;
    for (const auto& subtype_info : ime_info->subtypes) {
      VLOG(1) << "@@@@@          id=" << subtype_info->id
              << " locale=" << subtype_info->locale
              << " displayName=" << subtype_info->displayName;
    }
  }
  std::string ext_id{"org.chromium.arc.inputmethod.proxy"};
  if (!ime_engine_) {
    VLOG(1) << "@@@@@ create & init IME: ext_id=" << ext_id;
    std::unique_ptr<input_method::InputMethodEngineBase::Observer> observer(
        new ArcImeProxyObserver());
    ime_engine_.reset(new chromeos::InputMethodEngine());
    ime_engine_->Initialize(std::move(observer), ext_id.c_str(), profile_);
  }

  auto ime_state =
      chromeos::UserSessionManager::GetInstance()->GetDefaultIMEState(profile_);
  if (ime_infos.size() == 0) {
    VLOG(1) << "@@@@@ remove IME: ext_id=" << ext_id;
    ime_state->RemoveInputMethodExtension(ext_id);
    return;
  }

  arc_proxy_ime_id_ = "xkb::en:" + ext_id + ".1";
  ime_descriptors_.clear();
  for (const auto& ime_info : ime_infos) {
    std::vector<std::string> layouts{"us"};
    std::vector<std::string> languages{"en-US"};
    // for (const auto& subtype_info : ime_info->subtypes) {
    // }
    ime_descriptors_.push_back(chromeos::input_method::InputMethodDescriptor(
        ime_info->id, ime_info->displayName, ime_info->displayName, layouts,
        languages, false /* is_login_keyboard */, GURL() /* options_page_url */,
        GURL() /* input_view_url */));
  }
  ime_state->AddInputMethodExtension(ext_id, ime_descriptors_,
                                     ime_engine_.get());
}

}  // namespace arc
