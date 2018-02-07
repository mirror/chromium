// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/win/uninstall_app_controller.h"

#include <atlbase.h>
#include <wrl/client.h>

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/pattern.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/win/scoped_variant.h"
#include "chrome/browser/win/automation_controller.h"
#include "chrome/browser/win/ui_automation_util.h"
#include "chrome/installer/util/shell_util.h"

namespace {

// Makes sure only one instance lives at the same time.
UninstallAppController* g_instance = nullptr;

}  // namespace

class UninstallAppController::AutomationControllerDelegate
    : public AutomationController::Delegate {
 public:
  AutomationControllerDelegate(
      scoped_refptr<base::SequencedTaskRunner> controller_runner,
      const base::WeakPtr<UninstallAppController> controller,
      const base::string16& program_name);
  ~AutomationControllerDelegate() override;

  // AutomationController::Delegate:
  void OnInitialized(HRESULT result) override;
  void ConfigureCacheRequest(IUIAutomationCacheRequest* cache_request) override;
  void OnAutomationEvent(IUIAutomation* automation,
                         IUIAutomationElement* sender,
                         EVENTID event_id) override;
  void OnFocusChangedEvent(IUIAutomation* automation,
                           IUIAutomationElement* sender) override;

 private:
  // The task runner on which the UninstallAppController lives.
  scoped_refptr<base::SequencedTaskRunner> controller_runner_;

  // Only used to post callbacks to |controller_runner_|;
  const base::WeakPtr<UninstallAppController> controller_;

  const base::string16 program_name_;

  DISALLOW_COPY_AND_ASSIGN(AutomationControllerDelegate);
};

UninstallAppController::AutomationControllerDelegate::
    AutomationControllerDelegate(
        scoped_refptr<base::SequencedTaskRunner> controller_runner,
        const base::WeakPtr<UninstallAppController> controller,
        const base::string16& program_name)
    : controller_runner_(std::move(controller_runner)),
      controller_(controller),
      program_name_(program_name) {}

UninstallAppController::AutomationControllerDelegate::
    ~AutomationControllerDelegate() = default;

void UninstallAppController::AutomationControllerDelegate::OnInitialized(
    HRESULT result) {
  // Launch the Apps & Features settings page regardless of the |result| of the
  // initialization. An initialization failure only means that the program will
  // not be highlighted in the list of uninstallable programs.
  ShellUtil::LaunchUninstallAppsSettings();
}

void UninstallAppController::AutomationControllerDelegate::
    ConfigureCacheRequest(IUIAutomationCacheRequest* cache_request) {
  cache_request->AddPattern(UIA_ValuePatternId);
  cache_request->AddProperty(UIA_AutomationIdPropertyId);
  cache_request->AddProperty(UIA_IsWindowPatternAvailablePropertyId);
}

void UninstallAppController::AutomationControllerDelegate::OnAutomationEvent(
    IUIAutomation* automation,
    IUIAutomationElement* sender,
    EVENTID event_id) {}

bool FindSearchBoxElement(IUIAutomation* automation,
                          IUIAutomationElement* sender,
                          IUIAutomationElement** search_box) {
  // Create a condition that will include only elements with the right
  // automation id in the tree walker.
  base::win::ScopedVariant search_box_id(
      L"SystemSettings_StorageSense_AppSizesListFilter_DisplayStringValue");
  Microsoft::WRL::ComPtr<IUIAutomationCondition> condition;
  HRESULT result = automation->CreatePropertyCondition(
      UIA_AutomationIdPropertyId, search_box_id, condition.GetAddressOf());
  if (FAILED(result))
    return false;

  Microsoft::WRL::ComPtr<IUIAutomationTreeWalker> tree_walker;
  result =
      automation->CreateTreeWalker(condition.Get(), tree_walker.GetAddressOf());
  if (FAILED(result))
    return false;

  // Setup a cache request so that the element contains the property we need
  // afterwards.
  Microsoft::WRL::ComPtr<IUIAutomationCacheRequest> cache_request;
  result = automation->CreateCacheRequest(cache_request.GetAddressOf());
  if (FAILED(result))
    return false;
  cache_request->AddPattern(UIA_ValuePatternId);

  result = tree_walker->GetNextSiblingElementBuildCache(
      sender, cache_request.Get(), search_box);
  return SUCCEEDED(result) && *search_box;
}

void UninstallAppController::AutomationControllerDelegate::OnFocusChangedEvent(
    IUIAutomation* automation,
    IUIAutomationElement* sender) {
  base::string16 combo_box_id(
      GetCachedBstrValue(sender, UIA_AutomationIdPropertyId));
  if (combo_box_id != L"SystemSettings_AppsFeatures_AppControl_ComboBox")
    return;

  Microsoft::WRL::ComPtr<IUIAutomationElement> search_box;
  if (!FindSearchBoxElement(automation, sender, search_box.GetAddressOf()))
    return;

  Microsoft::WRL::ComPtr<IUIAutomationValuePattern> value_pattern;
  HRESULT result = search_box->GetCachedPatternAs(
      UIA_ValuePatternId, IID_PPV_ARGS(value_pattern.GetAddressOf()));
  if (FAILED(result))
    return;

  CComBSTR bstr(program_name_.c_str());
  value_pattern->SetValue(bstr);

  controller_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UninstallAppController::OnUninstallFinished,
                                controller_));
}

// static
void UninstallAppController::Launch(const base::string16& program_name) {
  // The instance handles its own lifetime.
  if (!g_instance)
    g_instance = new UninstallAppController(program_name);
}

UninstallAppController::UninstallAppController(
    const base::string16& program_name)
    : weak_ptr_factory_(this) {
  auto automation_controller_delegate =
      std::make_unique<AutomationControllerDelegate>(
          base::SequencedTaskRunnerHandle::Get(),
          weak_ptr_factory_.GetWeakPtr(), program_name);

  automation_controller_ = std::make_unique<AutomationController>(
      std::move(automation_controller_delegate));
}

UninstallAppController::~UninstallAppController() = default;

void UninstallAppController::OnUninstallFinished() {
  delete g_instance;
  g_instance = nullptr;
}
