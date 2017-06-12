// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/ime_controller_client.h"

#include "ash/ime/ime_controller.h"
#include "ash/public/interfaces/ime_info.mojom.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ime/chromeos/extension_ime_util.h"
#include "ui/base/ime/chromeos/input_method_descriptor.h"
#include "ui/base/ime/chromeos/input_method_util.h"

using chromeos::input_method::InputMethodDescriptor;
using chromeos::input_method::InputMethodManager;
using chromeos::input_method::InputMethodUtil;
using ui::ime::InputMethodMenuManager;

ImeControllerClient::ImeControllerClient(InputMethodManager* manager)
    : input_method_manager_(manager) {
  DCHECK(input_method_manager_);
  input_method_manager_->AddObserver(this);
  input_method_manager_->AddImeMenuObserver(this);
  InputMethodMenuManager::GetInstance()->AddObserver(this);

  //JAMES send to ash?
}

ImeControllerClient::~ImeControllerClient() {
  LOG(ERROR) << "JAMES del ImeControllerClient";
  InputMethodMenuManager::GetInstance()->RemoveObserver(this);
  input_method_manager_->RemoveImeMenuObserver(this);
  input_method_manager_->RemoveObserver(this);
}

// chromeos::input_method::InputMethodManager::Observer:
void ImeControllerClient::InputMethodChanged(InputMethodManager* manager,
                        Profile* profile,
                        bool show_message) {
  LOG(ERROR) << "JAMES InputMethodChanged";
  SendImesToAsh();
  SendImeMenuItemsToAsh();
}

// chromeos::input_method::InputMethodManager::ImeMenuObserver:
void ImeControllerClient::ImeMenuActivationChanged(bool is_active) {
  LOG(ERROR) << "JAMES ImeMenuActivationChanged";
  ash::ImeController* ime_controller = ash::Shell::Get()->ime_controller();
  ime_controller->ShowImeMenuOnShelf(is_active);
}

void ImeControllerClient::ImeMenuListChanged() {
  LOG(ERROR) << "JAMES ImeMenuListChanged";
  // SendImesToAsh();
}

void ImeControllerClient::ImeMenuItemsChanged(
    const std::string& engine_id,
    const std::vector<InputMethodManager::MenuItem>&
        items) {
  LOG(ERROR) << "JAMES ImeMenuItemsChanged";
}

// ui::ime::InputMethodMenuManager::Observer:
void ImeControllerClient::InputMethodMenuItemChanged(
    InputMethodMenuManager* manager) {
  LOG(ERROR) << "JAMES InputMethodMenuItemChanged";
  SendImesToAsh();//JAMES this seems weird
  SendImeMenuItemsToAsh();
}

ash::mojom::ImeInfo ImeControllerClient::GetAshImeInfo(
    const InputMethodDescriptor& ime) const {
  InputMethodUtil* util = input_method_manager_->GetInputMethodUtil();
  ash::mojom::ImeInfo info;
  info.id = ime.id();
  info.name = util->GetInputMethodLongName(ime);
  info.medium_name = util->GetInputMethodMediumName(ime);
  info.short_name = util->GetInputMethodShortName(ime);
  info.third_party = chromeos::extension_ime_util::IsExtensionIME(ime.id());
  return info;
}

void ImeControllerClient::SendImesToAsh() {
  LOG(ERROR) << "JAMES SendImesToAsh";
  ash::ImeController* ime_controller = ash::Shell::Get()->ime_controller();
  InputMethodManager::State* state = input_method_manager_->GetActiveIMEState().get();
  if (!state) {
    ime_controller->OnCurrentImeChanged(ash::mojom::ImeInfo());
    ime_controller->OnAvailableImesChanged(std::vector<ash::mojom::ImeInfo>(), false);
    return;
  }

  InputMethodDescriptor current_ime = state->GetCurrentInputMethod();
  ash::mojom::ImeInfo info = GetAshImeInfo(current_ime);
  info.selected = true;
  ime_controller->OnCurrentImeChanged(info);

  std::vector<ash::mojom::ImeInfo> imes;
  std::unique_ptr<std::vector<InputMethodDescriptor>> descriptors =
      state->GetActiveInputMethods();
  for (const InputMethodDescriptor& descriptor : *descriptors) {
    ash::mojom::ImeInfo info = GetAshImeInfo(descriptor);
    info.selected = descriptor.id() == current_ime.id();
    imes.push_back(info);
  }
  // Having a non-empty "allowed" list indicates that IMEs are managed.
  const bool managed_by_policy = !state->GetAllowedInputMethods().empty();
  ime_controller->OnAvailableImesChanged(imes, managed_by_policy);
}

void ImeControllerClient::SendImeMenuItemsToAsh() {
  std::vector<ash::mojom::ImeMenuItem> items;
  ui::ime::InputMethodMenuItemList menu_list =
      ui::ime::InputMethodMenuManager::GetInstance()
          ->GetCurrentInputMethodMenuItemList();
  for (size_t i = 0; i < menu_list.size(); ++i) {
    ash::mojom::ImeMenuItem item;
    item.key = menu_list[i].key;
    item.label = base::UTF8ToUTF16(menu_list[i].label);
    item.checked = menu_list[i].is_selection_item_checked;
    items.push_back(item);
  }
  LOG(ERROR) << "JAMES SendImeMenuItemsToAsh " << items.size();
  ash::Shell::Get()->ime_controller()->OnCurrentImeMenuItemsChanged(items);
}
