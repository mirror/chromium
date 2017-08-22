// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_win.h"

#include <activation.h>
#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_template_builder.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "content/public/browser/browser_thread.h"
#include "ui/message_center/notification.h"

namespace mswr = Microsoft::WRL;
namespace mswrw = Microsoft::WRL::Wrappers;

namespace winfoundtn = ABI::Windows::Foundation;
namespace winui = ABI::Windows::UI;
namespace winxml = ABI::Windows::Data::Xml;

using message_center::RichNotificationData;

namespace {

// Provides access to functions in combase.dll which may not be available on
// Windows 7. Loads functions dynamically at runtime to prevent library
// dependencies.
class CombaseFunctions {
 public:
  CombaseFunctions() = default;

  ~CombaseFunctions() {
    if (combase_dll_)
      ::FreeLibrary(combase_dll_);
  }

  bool LoadFunctions() {
    combase_dll_ = ::LoadLibrary(L"combase.dll");
    if (!combase_dll_)
      return false;

    get_factory_func_ = reinterpret_cast<decltype(&::RoGetActivationFactory)>(
        ::GetProcAddress(combase_dll_, "RoGetActivationFactory"));
    if (!get_factory_func_)
      return false;

    create_string_func_ = reinterpret_cast<decltype(&::WindowsCreateString)>(
        ::GetProcAddress(combase_dll_, "WindowsCreateString"));
    if (!create_string_func_)
      return false;

    return true;
  }

  HRESULT RoGetActivationFactory(HSTRING class_id,
                                 const IID& iid,
                                 void** out_factory) {
    DCHECK(get_factory_func_);
    return get_factory_func_(class_id, iid, out_factory);
  }

  HRESULT WindowsCreateString(const base::char16* src,
                              uint32_t len,
                              HSTRING* out_hstr) {
    DCHECK(create_string_func_);
    return create_string_func_(src, len, out_hstr);
  }

 private:
  HMODULE combase_dll_ = nullptr;

  decltype(&::RoGetActivationFactory) get_factory_func_ = nullptr;
  decltype(&::WindowsCreateString) create_string_func_ = nullptr;
};

CombaseFunctions* GetCombaseFunctions() {
  static CombaseFunctions* functions = new CombaseFunctions();
  return functions;
}

// Creates a HSTRING object for the given |value|. It must be freed manually,
// which should be done by attaching it to a mswrw::HString object.
HSTRING CreateHSTRING(const base::string16& value) {
  HSTRING string;

  HRESULT hr = GetCombaseFunctions()->WindowsCreateString(
      value.c_str(), static_cast<UINT32>(value.size()), &string);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create a HSTRING value for: " << value;
    return NULL;
  }

  return string;
}

// Templated wrapper for winfoundtn::GetActivationFactory().
template <unsigned int size, typename T>
HRESULT CreateActivationFactory(wchar_t const (&class_name)[size], T** object) {
  mswrw::HStringReference ref_class_name(class_name);
  return GetCombaseFunctions()->RoGetActivationFactory(ref_class_name.Get(),
                                                       IID_PPV_ARGS(object));
}

// TODO(finnur): Make this a member function of the class and unit test it.
// Obtain an IToastNotification interface from a given XML (provided by the
// NotificationTemplateBuilder).
HRESULT GetToastNotification(
    const NotificationTemplateBuilder& notification_template_builder,
    winui::Notifications::IToastNotification** toast_notification) {
  mswr::ComPtr<winxml::Dom::IXmlDocumentIO> document_io;
  mswrw::HStringReference ref_class_name(
      RuntimeClass_Windows_Data_Xml_Dom_XmlDocument);

  HRESULT hr =
      Windows::Foundation::ActivateInstance(ref_class_name.Get(), &document_io);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to activate the XML Document";
    return hr;
  }

  base::string16 notification_template =
      notification_template_builder.GetNotificationTemplate();
  mswrw::HStringReference ref_template(notification_template.c_str());

  hr = document_io->LoadXml(ref_template.Get());
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to load the template's XML into the document";
    return hr;
  }

  mswr::ComPtr<winxml::Dom::IXmlDocument> document;
  hr = document_io.As<winxml::Dom::IXmlDocument>(&document);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to get as XMLDocument";
    return hr;
  }

  mswr::ComPtr<winui::Notifications::IToastNotificationFactory>
      toast_notification_factory;

  hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotification,
      toast_notification_factory.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create the IToastNotificationFactory";
    return hr;
  }

  hr = toast_notification_factory->CreateToastNotification(document.Get(),
                                                           toast_notification);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create the IToastNotification";
    return hr;
  }

  return S_OK;
}

}  // namespace

// static
NotificationPlatformBridge* NotificationPlatformBridge::Create() {
  return new NotificationPlatformBridgeWin();
}

NotificationPlatformBridgeWin::NotificationPlatformBridgeWin() {
  GetCombaseFunctions()->LoadFunctions();
}

NotificationPlatformBridgeWin::~NotificationPlatformBridgeWin() = default;

void NotificationPlatformBridgeWin::Display(
    NotificationCommon::Type notification_type,
    const std::string& notification_id,
    const std::string& profile_id,
    bool incognito,
    const Notification& notification) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  mswr::ComPtr<winui::Notifications::IToastNotificationManagerStatics>
      toast_manager;

  HRESULT hr = CreateActivationFactory(
      RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
      toast_manager.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create the ToastNotificationManager";
    return;
  }

  base::string16 browser_model_id =
      ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());

  mswrw::HString application_id;
  application_id.Attach(CreateHSTRING(browser_model_id));

  mswr::ComPtr<winui::Notifications::IToastNotifier> notifier;

  hr =
      toast_manager->CreateToastNotifierWithId(application_id.Get(), &notifier);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to create the ToastNotifier";
    return;
  }

  std::unique_ptr<NotificationTemplateBuilder> notification_template =
      NotificationTemplateBuilder::Build(notification_id, notification);
  mswr::ComPtr<winui::Notifications::IToastNotification> toast;
  hr = GetToastNotification(*notification_template, &toast);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to get a toast notification";
    return;
  }

  hr = notifier->Show(toast.Get());
  if (FAILED(hr))
    LOG(ERROR) << "Unable to display the notification";
}

void NotificationPlatformBridgeWin::Close(const std::string& profile_id,
                                          const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(peter): Implement the ability to close notifications.
}

void NotificationPlatformBridgeWin::GetDisplayed(
    const std::string& profile_id,
    bool incognito,
    const GetDisplayedNotificationsCallback& callback) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto displayed_notifications = base::MakeUnique<std::set<std::string>>();
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(callback, base::Passed(&displayed_notifications),
                 false /* supports_synchronization */));
}

void NotificationPlatformBridgeWin::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::move(callback).Run(true);
}
