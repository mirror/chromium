// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_win.h"

#include <activation.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <memory>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#include "chrome/browser/browser_process.h"
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

using base::win::ScopedHString;
using message_center::RichNotificationData;

namespace {

typedef winfoundtn::ITypedEventHandler<winui::Notifications::ToastNotification*,
                                       IInspectable*>
    ToastActivatedHandler;

typedef winfoundtn::ITypedEventHandler<
    winui::Notifications::ToastNotification*,
    winui::Notifications::ToastDismissedEventArgs*>
    ToastDismissedHandler;

typedef winfoundtn::ITypedEventHandler<
    winui::Notifications::ToastNotification*,
    winui::Notifications::ToastFailedEventArgs*>
    ToastFailedHandler;

// Templated wrapper for winfoundtn::GetActivationFactory().
template <unsigned int size, typename T>
HRESULT CreateActivationFactory(wchar_t const (&class_name)[size], T** object) {
  ScopedHString ref_class_name =
      ScopedHString::Create(base::StringPiece16(class_name, size - 1));
  return base::win::RoGetActivationFactory(ref_class_name.get(),
                                           IID_PPV_ARGS(object));
}

// TODO(finnur): Make this a member function of the class and unit test it.
// Obtain an IToastNotification interface from a given XML (provided by the
// NotificationTemplateBuilder).
HRESULT GetToastNotification(
    const NotificationTemplateBuilder& notification_template_builder,
    winui::Notifications::IToastNotification** toast_notification) {
  *toast_notification = nullptr;

  ScopedHString ref_class_name =
      ScopedHString::Create(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument);
  mswr::ComPtr<IInspectable> inspectable;
  HRESULT hr =
      base::win::RoActivateInstance(ref_class_name.get(), &inspectable);
  if (FAILED(hr)) {
    LOG(ERROR) << "Unable to activate the XML Document";
    return hr;
  }

  mswr::ComPtr<winxml::Dom::IXmlDocumentIO> document_io;
  hr = inspectable.As<winxml::Dom::IXmlDocumentIO>(&document_io);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get XmlDocument as IXmlDocumentIO";
    return hr;
  }

  base::string16 notification_template =
      notification_template_builder.GetNotificationTemplate();
  ScopedHString ref_template = ScopedHString::Create(notification_template);
  hr = document_io->LoadXml(ref_template.get());
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
  com_functions_initialized_ = base::win::ResolveCoreWinRTDelayload() &&
                               ScopedHString::ResolveCoreWinRTStringDelayload();
}

// This callback is invoked when user clicks on the notification toast. This can
// occur at any time in Chrome's cycle, so we need to be robust.
HRESULT NotificationPlatformBridgeWin::ToastEventHandler::OnActivated(
    winui::Notifications::IToastNotification* notification,
    IInspectable* /* inspectable */) {
  PostHandlerOnUIThread(EVENT_TYPE_ACTIVATED, notification);
  return S_OK;
}

HRESULT NotificationPlatformBridgeWin::ToastEventHandler::OnDismissed(
    winui::Notifications::IToastNotification* notification,
    winui::Notifications::IToastDismissedEventArgs* /* args */) {
  PostHandlerOnUIThread(EVENT_TYPE_DISMISSED, notification);
  return S_OK;
}

HRESULT NotificationPlatformBridgeWin::ToastEventHandler::OnFailed(
    winui::Notifications::IToastNotification* notification,
    winui::Notifications::IToastFailedEventArgs* /* args */) {
  PostHandlerOnUIThread(EVENT_TYPE_FAILED, notification);
  return S_OK;
}

// static
void NotificationPlatformBridgeWin::ToastEventHandler::PostHandlerOnUIThread(
    EventType type,
    winui::Notifications::IToastNotification* notification) {
  // Extract the notifiation id and profiler id, which is empty now.
  std::string notification_id = "";
  std::string profile_id = "";

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(
          &NotificationPlatformBridgeWin::ToastEventHandler::HandleOnUIThread,
          type, notification_id, profile_id));
}

// static
void NotificationPlatformBridgeWin::ToastEventHandler::HandleOnUIThread(
    EventType type,
    const std::string& notification_id,
    const std::string& profile_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Get the NotificationPlatformBridgeWin singleton.
  if (!g_browser_process) {
    MessageBoxW(NULL, L"no g_browser_process", L"Title", MB_OK);
    return;
  }
  MessageBoxW(NULL, L"get g_browser_process", L"Title", MB_OK);
  NotificationPlatformBridge* notification_bridge =
      g_browser_process->notification_platform_bridge();
  if (!notification_bridge)
    return;
  MessageBoxW(NULL, L"get notification_bridge", L"Title", MB_OK);
  NotificationPlatformBridgeWin* notification_bridge_win =
      static_cast<NotificationPlatformBridgeWin*>(notification_bridge);

  switch (type) {
    case EVENT_TYPE_ACTIVATED: {
      notification_bridge_win->OnClickEvent(notification_id, profile_id);
      notification_bridge_win->OnCloseEvent(notification_id, profile_id);
      break;
    }
    case EVENT_TYPE_DISMISSED: {
      notification_bridge_win->OnCloseEvent(notification_id, profile_id);
      break;
    }
    case EVENT_TYPE_FAILED: {
      break;
    }
  }
}

NotificationPlatformBridgeWin::ToastEventHandler
    NotificationPlatformBridgeWin::toast_event_handler_;

NotificationPlatformBridgeWin::~NotificationPlatformBridgeWin() = default;

void NotificationPlatformBridgeWin::Display(
    NotificationCommon::Type notification_type,
    const std::string& notification_id,
    const std::string& profile_id,
    bool incognito,
    const Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  // TODO(finnur): Move this to a RoInitialized thread, as per crbug.com/761039.
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
  ScopedHString application_id = ScopedHString::Create(browser_model_id);
  mswr::ComPtr<winui::Notifications::IToastNotifier> notifier;
  hr =
      toast_manager->CreateToastNotifierWithId(application_id.get(), &notifier);
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

  // Add a few proper handlers

  auto activated_handler = mswr::Callback<ToastActivatedHandler>(
      &toast_event_handler_,
      &NotificationPlatformBridgeWin::ToastEventHandler::OnActivated);
  EventRegistrationToken activated_token;
  hr = toast->add_Activated(activated_handler.Get(), &activated_token);
  if (FAILED(hr))
    return;

  auto dismissed_handler = mswr::Callback<ToastDismissedHandler>(
      &toast_event_handler_,
      &NotificationPlatformBridgeWin::ToastEventHandler::OnDismissed);
  EventRegistrationToken dismissed_token;
  hr = toast->add_Dismissed(dismissed_handler.Get(), &dismissed_token);
  if (FAILED(hr))
    return;

  auto failed_handler = mswr::Callback<ToastFailedHandler>(
      &toast_event_handler_,
      &NotificationPlatformBridgeWin::ToastEventHandler::OnFailed);
  EventRegistrationToken failed_token;
  hr = toast->add_Failed(failed_handler.Get(), &failed_token);
  if (FAILED(hr))
    return;

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
  auto displayed_notifications = std::make_unique<std::set<std::string>>();
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(callback, base::Passed(&displayed_notifications),
                 false /* supports_synchronization */));
}

void NotificationPlatformBridgeWin::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  std::move(callback).Run(com_functions_initialized_);
}

void NotificationPlatformBridgeWin::OnClickEvent(
    const std::string& notification_id,
    const std::string& profile_id) {
  // TODO(chengx): implement
  MessageBoxW(NULL, L"OnClickEvent()", L"Title", MB_OK);
}

void NotificationPlatformBridgeWin::OnCloseEvent(
    const std::string& notification_id,
    const std::string& profile_id) {
  // TODO(chengx): implement
  MessageBoxW(NULL, L"OnCloseEvent()", L"Title", MB_OK);
}
