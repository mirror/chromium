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
#include "components/crx_file/id_util.h"
#include "ui/base/ime/chromeos/extension_ime_util.h"
#include "ui/base/ime/chromeos/input_method_util.h"
#include "url/gurl.h"

namespace arc {

class ArcImeManagerBridge::ArcImeProxyObserver
    : public input_method::InputMethodEngineBase::Observer {
 public:
    ArcImeProxyObserver(ArcImeManagerBridge* arc_ime_manager)
       : arc_ime_manager_(arc_ime_manager) {}
  ~ArcImeProxyObserver() {}

  void OnActivate(const std::string& engine_id) override {
    VLOG(1) << "@@@@@ OnActivate: engine_id=" << engine_id;
    arc_ime_manager_->SwitchImeTo(engine_id);
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
    arc_ime_manager_->SwitchImeTo(
        "org.chromium.arc.ime/.ArcInputMethodService");
    VLOG(1) << "@@@@@  switch to org.chromium.arc.ime/.ArcInputMethodService";
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

private:
    ArcImeManagerBridge* const arc_ime_manager_;
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

// static
ArcImeManagerBridge* ArcImeManagerBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcImeManagerBridgeFactory::GetForBrowserContext(context);
}

ArcImeManagerBridge::ArcImeManagerBridge(content::BrowserContext* context,
                                         ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      profile_(Profile::FromBrowserContext(context)),
      proxy_ime_engine_(new chromeos::InputMethodEngine()) {
  arc_bridge_service_->ime_manager()->AddObserver(this);
  chromeos::input_method::InputMethodManager::Get()->AddObserver(this);

  proxy_ime_extension_id_ =
      crx_file::id_util::GenerateId("org.chromium.arc.inputmethod.proxy");
  std::unique_ptr<ArcImeProxyObserver> observer(new ArcImeProxyObserver(this));
  proxy_ime_engine_observer_ = observer.get();
  proxy_ime_engine_->Initialize(
      std::move(observer), proxy_ime_extension_id_.c_str(), profile_);
  VLOG(1) << "@@@@@ ArcImeManagerBridge created";
}

ArcImeManagerBridge::~ArcImeManagerBridge() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  chromeos::input_method::InputMethodManager::Get()->RemoveObserver(this);
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
    const std::string& locale) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  VLOG(1) << "@@@@@ NotifyDefaultImeChanged: ime_id=" << ime_id
          << " locale=" << locale;
  current_arc_ime_id_ = ime_id;
  current_arc_ime_locale_ = locale;
}

void ArcImeManagerBridge::NotifyImeInfos(
    std::vector<mojom::ImeInfoPtr> ime_infos) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // TODO: Create proxy IMEs' descriptors.
  VLOG(1) << "@@@@@ NotifyImeInfos: ";
  for (const auto& ime_info : ime_infos) {
    VLOG(1) << "@@@@@     " << (ime_info->enabled ? "*" : " ")
            << " imeId=" << ime_info->imeId
            << " displayName=" << ime_info->displayName;
    for (const auto& locale : ime_info->locales) {
      VLOG(1) << "@@@@@          locale=" << locale;
    }
  }

  auto imm_state =
      chromeos::UserSessionManager::GetInstance()->GetDefaultIMEState(profile_);
  imm_state->RemoveInputMethodExtension(proxy_ime_extension_id_);
  if (ime_infos.size() == 0) {
      return;
  }

  proxy_ime_descriptors_.clear();
  for (const auto& ime_info : ime_infos) {
    std::vector<std::string> layouts{"us"};
    std::vector<std::string> languages;
    for (const auto& locale: ime_info->locales) {
      // TODO: FIX ME
      languages.push_back(locale);
    }
    const std::string input_method_id =
        chromeos::extension_ime_util::GetContainerInputMethodID(
            proxy_ime_extension_id_, ime_info->imeId);
    const std::string& indicator = (ime_info->displayName.size() < 4)
        ? ime_info->displayName
        : ime_info->displayName.substr(0, 4);
    proxy_ime_descriptors_.push_back(
        chromeos::input_method::InputMethodDescriptor(
            input_method_id,
            ime_info->displayName,
            indicator,
            layouts,
            languages,
            false /* is_login_keyboard */,
            GURL() /* options_page_url */,
            GURL() /* input_view_url */));
  }
  imm_state->AddInputMethodExtension(proxy_ime_extension_id_,
                                     proxy_ime_descriptors_,
                                     proxy_ime_engine_.get());
  for (const auto& ime_info : ime_infos) {
    if (!ime_info->enabled) {
      continue;
    }
    const std::string input_method_id =
        chromeos::extension_ime_util::GetContainerInputMethodID(
            proxy_ime_extension_id_, ime_info->imeId);
    imm_state->EnableInputMethod(input_method_id);
  }
}

void ArcImeManagerBridge::ActiveInputMethodsChanged(
    chromeos::input_method::InputMethodManager* manager) {
    VLOG(1) << "@@@@@ ActiveInputMethodsChanged:";
    std::set<std::string> active_container_imes;
    for (const auto& id : manager->GetActiveIMEState()->GetActiveInputMethodIds()) {
        if (chromeos::extension_ime_util::IsContainerExtensionIME(id)) {
            active_container_imes.insert(id);
        }
    }
    for (const auto& descriptor : proxy_ime_descriptors_) {
        bool enable = active_container_imes.find(descriptor.id()) != active_container_imes.end();
        EnableIme(descriptor.id(), enable);
    }
}

void ArcImeManagerBridge::SwitchImeTo(const std::string& ime_id) {
    VLOG(1) << "@@@@@ SwitchImeTo: ime_id=" << ime_id;
    mojom::ImeManagerInstance* ime_manager_instance =
        ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->ime_manager(), SwitchImeTo);
    DCHECK(ime_manager_instance);
    ime_manager_instance->SwitchImeTo(ime_id);
}
            
void ArcImeManagerBridge::EnableIme(const std::string& ime_id, bool enable) {
    VLOG(1) << "@@@@@ EnableIme: ime_id=" << ime_id << (enable ? " enable" : " disable");
    mojom::ImeManagerInstance* ime_manager_instance =
        ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->ime_manager(), EnableIme);
    DCHECK(ime_manager_instance);
    ime_manager_instance->EnableIme(ime_id, enable);
}

}  // namespace arc
